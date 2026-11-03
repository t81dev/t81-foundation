"""Static artifact and fixture-accounting validators. No resource sampling API calls."""
import hashlib
import math
import struct

MAX_ARTIFACT = 128 * 1024 * 1024
MAX_SECTIONS = 4096


def require(value, reason):
    if not value:
        raise ValueError(reason)


def rss_kib(raw, system):
    require(isinstance(raw, (int, float)) and not isinstance(raw, bool) and math.isfinite(raw) and raw >= 0,
            "invalid raw RSS fixture")
    if system == "Linux":
        return raw
    if system == "Darwin":
        return raw / 1024
    raise ValueError("unsupported RSS platform")


def selector_scope(selector, worker_pid, observed_pid, *, reaped=False):
    require(type(worker_pid) is int and worker_pid > 0 and type(observed_pid) is int and observed_pid > 0,
            "invalid worker identity")
    if selector == "RUSAGE_SELF":
        require(worker_pid == observed_pid, "SELF cannot describe another PID")
        return "calling worker, all its threads; lifetime high-water RSS"
    if selector == "WAITED_CHILD":
        require(worker_pid == observed_pid and reaped, "specific waited/reaped PID required")
        return "specific reaped worker; never a difference of aggregate child peaks"
    if selector == "RUSAGE_CHILDREN":
        raise ValueError("aggregate waited descendants are not a per-worker observer")
    raise ValueError("unsupported accounting selector")


def selector_bindings(system):
    if system not in ("Linux", "Darwin"):
        return {"status":"UNSUPPORTED", "reason":"Linux/macOS accounting constants only"}
    import resource
    require(resource.RUSAGE_SELF == 0 and resource.RUSAGE_CHILDREN == -1,
            "unexpected platform selector constants")
    return {"status":"SUPPORTED", "RUSAGE_SELF":resource.RUSAGE_SELF,
            "RUSAGE_CHILDREN":resource.RUSAGE_CHILDREN, "samples_collected":False}


def validate_lifecycle(records):
    require(len(records) == 2, "require exactly spawn and terminal records")
    start, end = records
    require(start["event"] == "spawned" and end["event"] == "exited", "invalid lifecycle order")
    require(type(start["pid"]) is int and start["pid"] > 0 and start["pid"] == end["pid"], "PID mismatch")
    require(type(start["parent_pid"]) is int and start["parent_pid"] > 0, "parent PID missing")
    require(type(end["returncode"]) is int, "terminal status missing")
    return {"pid":start["pid"], "parent_pid":start["parent_pid"], "complete":True,
            "returncode":end["returncode"], "resource_samples":[]}


def _range(data, offset, size):
    require(0 <= offset <= len(data) and 0 <= size <= len(data)-offset, "artifact range outside file")
    return data[offset:offset+size]


def _unpack(fmt, data, offset):
    return struct.unpack(fmt, _range(data, offset, struct.calcsize(fmt)))


def _name(raw):
    return raw.split(b"\0",1)[0].decode("utf-8", "backslashreplace")


def artifact_metadata(data):
    require(len(data) <= MAX_ARTIFACT, "artifact exceeds static parser bound")
    result = {"sha256":hashlib.sha256(data).hexdigest(), "file_bytes":len(data),
              "parser":"base01-static-v1", "status":"UNSUPPORTED", "sections":[], "segments":[]}
    if data.startswith(b"\x7fELF"):
        if len(data) < 16:
            raise ValueError("truncated ELF identification")
        if data[4:7] != b"\x02\x01\x01":
            result["reason"] = "only ELF64 little-endian version 1 is supported"
            return result
        h = _unpack("<16sHHIQQQIHHHHHH", data, 0)
        _, etype, machine, version, entry, phoff, shoff, flags, ehsize, phsize, phnum, shsize, shnum, shstr = h
        require(version == 1 and ehsize == 64, "invalid ELF64 header")
        if (shnum == 0 and shoff != 0) or shstr == 0xffff or phnum == 0xffff:
            result["reason"] = "extended ELF numbering unsupported"
            return result
        require(shnum <= MAX_SECTIONS and phnum <= MAX_SECTIONS, "ELF table bound")
        if phnum:
            require(phsize == 56 and phoff >= 64, "invalid ELF program table")
            _range(data, phoff, phsize*phnum)
            for i in range(phnum):
                typ, pflags, offset, addr, _, file_size, mem_size, align = _unpack("<IIQQQQQQ",data,phoff+i*phsize)
                _range(data,offset,file_size)
                if typ == 1:
                    require(file_size <= mem_size, "ELF load file size exceeds memory size")
                result["segments"].append({"type":typ,"flags":pflags,"file_offset":offset,
                    "file_bytes":file_size,"memory_bytes":mem_size,"address":addr,"alignment":align})
        if shnum:
            require(shsize == 64 and shoff >= 64 and shstr < shnum, "invalid ELF section table")
            _range(data,shoff,shsize*shnum)
            headers = [_unpack("<IIQQQQIIQQ",data,shoff+i*shsize) for i in range(shnum)]
            if shstr == 0:
                names = b"\0"
            else:
                require(headers[shstr][1] == 3, "ELF name table is not STRTAB")
                names = _range(data,headers[shstr][4],headers[shstr][5])
            for header in headers:
                no,typ,sflags,addr,offset,size,_,_,align,_ = header
                end = names.find(b"\0",no)
                require(no < len(names) and end >= no and end-no <= 4096, "invalid ELF section name")
                name = _name(names[no:end])
                file_size = 0 if typ == 8 else size  # SHT_NOBITS has no file payload.
                if file_size:
                    _range(data,offset,file_size)
                result["sections"].append({"name":name,"type":typ,"flags":sflags,
                    "address":addr,"file_offset":offset,"file_bytes":file_size,
                    "memory_bytes":size,"alignment":align})
        else:
            require(shoff == 0 and shstr == 0, "inconsistent absent ELF section table")
        result.update(status="SUPPORTED",format="ELF64-LE",machine=machine,object_type=etype)
        return result
    if data[:4] == b"\xcf\xfa\xed\xfe":
        _, cpu, subtype, filetype, count, command_bytes, flags, _ = _unpack("<IiiIIIII",data,0)
        require(count <= MAX_SECTIONS, "Mach-O command count bound")
        _range(data,32,command_bytes)
        cursor = 32
        for _ in range(count):
            command, size = _unpack("<II",data,cursor)
            require(size >= 8 and size % 8 == 0 and cursor+size <= 32+command_bytes,
                    "invalid Mach-O load command")
            if command == 0x19:  # LC_SEGMENT_64
                _,_,segraw,addr,mem_size,offset,file_size,maxprot,initprot,nsects,segflags = _unpack("<II16sQQQQiiII",data,cursor)
                require(nsects <= MAX_SECTIONS and size == 72+80*nsects, "Mach-O section count/size")
                _range(data,offset,file_size)
                require(file_size <= mem_size, "Mach-O file size exceeds memory size")
                segment = _name(segraw)
                result["segments"].append({"name":segment,"address":addr,"memory_bytes":mem_size,
                    "file_offset":offset,"file_bytes":file_size,"maxprot":maxprot,"initprot":initprot,"flags":segflags})
                for i in range(nsects):
                    secraw,ssegraw,saddr,ssize,soff,align,reloff,nreloc,sflags,_,_,_ = _unpack("<16s16sQQIIIIIIII",data,cursor+72+i*80)
                    require(_name(ssegraw) == segment, "Mach-O section segment mismatch")
                    require(addr <= saddr <= addr+mem_size and ssize <= addr+mem_size-saddr, "Mach-O section outside VM segment")
                    typ = sflags & 0xff
                    zero = typ in (1,0xc,0x12)  # zero-fill, GB zero-fill, TLS zero-fill
                    if not zero:
                        _range(data,soff,ssize)
                        require(offset <= soff and soff+ssize <= offset+file_size, "Mach-O section outside file segment")
                    if nreloc:
                        _range(data,reloff,nreloc*8)
                    result["sections"].append({"name":_name(secraw),"segment":segment,
                        "type":typ,"flags":sflags,"address":saddr,"memory_bytes":ssize,
                        "file_offset":soff,"file_bytes":0 if zero else ssize,"alignment_exponent":align})
                    require(len(result["sections"]) <= MAX_SECTIONS, "Mach-O aggregate section bound")
            cursor += size
        require(cursor == 32+command_bytes, "Mach-O command table length mismatch")
        result.update(status="SUPPORTED",format="Mach-O64-LE",machine=cpu,subtype=subtype,object_type=filetype)
        return result
    result["reason"] = "unknown, archive, fat, 32-bit or unsupported-endian artifact"
    return result

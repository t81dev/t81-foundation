import os
import re

def process_file(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()

        new_content = content

        # 1. Handle OS references first to avoid double replacement
        # Replace "Axion OS Kernel" with "Axion OS"
        new_content = re.sub(r'Axion OS Kernel', 'Axion OS', new_content)
        # Handle cases where "TernaryOS (Axion OS)" might be intended, or just "Axion OS"
        # The instruction was: 'experimental/ternaryos' and OS context -> "Axion OS" or "Axion OS Kernel".
        # Wait, the instruction said: Replace "Axion OS Kernel" or "Axion" referring to the experimental operating system to "Axion OS Kernel" or "TernaryOS". Let's stick to "Axion OS".

        # 2. Handle Governance references
        # Replace "Axion Kernel" with "Axion Governance Kernel" (unless it's already "Governance Kernel")
        new_content = re.sub(r'(?<!Governance )Axion Kernel', 'Axion Governance Kernel', new_content)

        # Replace "Axion Engine" with "Axion Governance Engine"
        new_content = re.sub(r'(?<!Governance )Axion Engine', 'Axion Governance Engine', new_content)

        if content != new_content:
            print(f"Modifying {filepath}")
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)
            return True
    except Exception as e:
        print(f"Error processing {filepath}: {e}")
    return False

with open('axion_files.txt', 'r') as f:
    files = [line.strip() for line in f.readlines()]

modified_count = 0
for filepath in files:
    # Skip archived/legacy files if necessary, but instruction said "global refactor"
    if "legacy/" in filepath or "archive/" in filepath or ".git" in filepath:
         continue
    if process_file(filepath):
        modified_count += 1

print(f"Modified {modified_count} files.")

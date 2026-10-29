// EXP-T81-COMP3-01: binary-host software trichotomy experiment.
// No native ternary hardware claims. Linux perf_event_open required for measurements.
#include <algorithm>
#include <array>
#include <chrono>
#include <compare>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
using i64=int64_t; using u64=uint64_t; using u128=unsigned __int128;
struct Pair {i64 a,b;};
static volatile u64 sink=0;
struct PCG {
  u128 state=0, inc=3;
  static constexpr u128 mult=(u128(0x2360ED051FC65DA4ULL)<<64)|0x4385C511560B53A9ULL;
  PCG(){state=state*mult+inc;state+=((u128)0x5438315F434D5033ULL<<64)|0x5F53454544303131ULL;state=state*mult+inc;}
  u64 next(){u128 old=state;state=old*mult+inc;u64 x=(u64)(old>>64)^(u64)old;unsigned r=(unsigned)(old>>122);return (x>>r)|(x<<((-r)&63));}
  u64 bounded(u64 n){u64 t=-n%n;for(;;){u64 x=next();if(x>=t)return x%n;}}
  double prob(){return (next()>>11)*0x1.0p-53;}
};
static int oracle(i64 a,i64 b){return (a>b)-(a<b);}
__attribute__((noinline)) static int cmp3_s64(i64 a,i64 b){return (a>b)-(a<b);}
__attribute__((noinline)) static u64 payload(i64 a,i64 b,int id){return (u64(a)^((u64)b<<1))+(u64)(id+2);}
static std::array<Pair,20> boundaries(){using L=std::numeric_limits<i64>;return {{{0,0},{42,42},{-42,-42},{L::max(),L::max()},{L::min(),L::min()},{L::min(),L::max()},{L::max(),L::min()},{100,101},{101,100},{-101,-100},{-100,-101},{-1,0},{0,-1},{0,1},{1,0},{-1,1},{1,-1},{9007199254740993LL,9007199254740994LL},{9007199254740994LL,9007199254740993LL},{L::max()-1,L::max()}}};}
static std::vector<Pair> corpus(int pattern,size_t n){
 PCG g;std::vector<Pair> v;v.reserve(n);
 const i64 low=-(1LL<<62);
 for(size_t i=0;i<n;i++){
  double u=g.prob();u64 k=g.bounded(1ULL<<30)+1;
  if(pattern==1){
   i64 x;
   if(u<0.2)x=-100-(i64)g.bounded(1000)-1;
   else if(u<0.3)x=-100+(i64)g.bounded(201);
   else x=100+(i64)g.bounded(1000)+1;
   // Normalize policy verdict to an identical three-way integer comparison.
   int verdict=x < -100 ? -1 : (x>100 ? 1 : 0);
   v.push_back({verdict,0});continue;
  }
  i64 a=low+(i64)g.bounded(1ULL<<63);
  double cut=pattern==0?0.475:1.0/3.0, upper=pattern==0?0.525:2.0/3.0;
  i64 b=u<cut?a+(i64)k:u<upper?a:a-(i64)k;
  v.push_back({a,b});
 }
 return v;
}
static u64 dispatch(const Pair& x,int mode){
 int c=0;
 switch(mode){
 case 0: {int z=cmp3_s64(x.a,x.b);c=z;break;}
 case 1: {if(x.a<x.b)c=-1;else if(x.a==x.b)c=0;else c=1;break;}
 case 2: c=(x.a>x.b)-(x.a<x.b);break;
 case 3: {auto z=x.a<=>x.b;c=z<0?-1:z>0?1:0;break;}
 case 4: {switch(x.a==x.b?0:x.a<x.b?-1:1){case -1:c=-1;break;case 0:c=0;break;default:c=1;}}break;
 }
 return payload(x.a,x.b,c);
}
struct Counters{int fd_i=-1,fd_b=-1;
 static int open_counter(u64 config){perf_event_attr a{};a.type=PERF_TYPE_HARDWARE;a.size=sizeof(a);a.config=config;a.disabled=1;a.exclude_kernel=1;a.exclude_hv=1;return (int)syscall(__NR_perf_event_open,&a,0,-1,-1,0);}
 Counters(){fd_i=open_counter(PERF_COUNT_HW_INSTRUCTIONS);fd_b=open_counter(PERF_COUNT_HW_BRANCH_MISSES);}
 ~Counters(){if(fd_i>=0)close(fd_i);if(fd_b>=0)close(fd_b);}
 bool available()const{return fd_i>=0&&fd_b>=0;}
 void start(){for(int fd:{fd_i,fd_b}){ioctl(fd,PERF_EVENT_IOC_RESET,0);ioctl(fd,PERF_EVENT_IOC_ENABLE,0);}}
 std::pair<u64,u64> stop(){for(int fd:{fd_i,fd_b})ioctl(fd,PERF_EVENT_IOC_DISABLE,0);u64 i=0,b=0;if(read(fd_i,&i,8)!=8||read(fd_b,&b,8)!=8)throw std::runtime_error("PMU_READ_FAILURE");return {i,b};}
};
int main(int argc,char**argv){
 size_t n=10000000;int reps=10;bool selftest=false;
 for(int j=1;j<argc;j++){std::string arg=argv[j];if(arg=="--selftest")selftest=true;else if(arg=="--n"&&j+1<argc)n=std::stoull(argv[++j]);else if(arg=="--reps"&&j+1<argc)reps=std::stoi(argv[++j]);else {fprintf(stderr,"unknown argument: %s\n",argv[j]);return 2;}}
 PCG g;const u64 expected[]={0x854CAFBD0A540778ULL,0x3D174C3776108E7DULL,0xA8398F10D0F2976CULL,0x722D0D72D6D3EA65ULL};
 for(u64 x:expected){u64 got=g.next();if(got!=x){fprintf(stderr,"BLOCKED PCG_VECTOR_MISMATCH expected=%016llx got=%016llx\n",(unsigned long long)x,(unsigned long long)got);return 3;}}
 for(auto x:boundaries()){int o=oracle(x.a,x.b);if(cmp3_s64(x.a,x.b)!=o){puts("BLOCKED CORRECTNESS_FAILURE");return 4;}for(int m=0;m<5;m++)if(dispatch(x,m)!=payload(x.a,x.b,o)){puts("BLOCKED PAYLOAD_PARITY");return 4;}}
 puts("BOUNDARY_AND_PCG_VECTORS_PASS");
 if(selftest)return 0;
 if(n>10000000||reps>10||reps<1){puts("BLOCKED BUDGET_PARAMETERS");return 5;}
 Counters perf;if(!perf.available()){puts("BLOCKED PMU_UNAVAILABLE");return 6;}
 puts("pattern,mode,repetition,instructions,branch_misses,elapsed_ns,checksum");
 auto begin=std::chrono::steady_clock::now();
 for(int p=0;p<3;p++){auto data=corpus(p,n);for(auto x:data)if(cmp3_s64(x.a,x.b)!=oracle(x.a,x.b)){puts("BLOCKED CORRECTNESS_FAILURE");return 4;}
  for(int m=0;m<5;m++){for(int r=0;r<reps;r++){
   u64 sum=0;perf.start();auto t0=std::chrono::steady_clock::now();
   for(const auto& x:data)sum+=dispatch(x,m);
   auto t1=std::chrono::steady_clock::now();auto [inst,miss]=perf.stop();sink=sink^sum;
   printf("%d,%d,%d,%llu,%llu,%lld,%llu\n",p,m,r,(unsigned long long)inst,(unsigned long long)miss,(long long)std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count(),(unsigned long long)sum);
   if(std::chrono::steady_clock::now()-begin>std::chrono::minutes(10)){puts("BLOCKED BUDGET_EXCEEDED");return 7;}
  }}
 }
 return 0;
}

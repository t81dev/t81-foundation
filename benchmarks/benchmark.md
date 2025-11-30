2025-11-30T16:47:47-05:00
Running ./benchmark_runner
Run on (8 X 24 MHz CPU s)
CPU Caches:
  L1 Data 64 KiB
  L1 Instruction 128 KiB
  L2 Unified 4096 KiB (x8)
Load Average: 2.86, 2.64, 2.45
----------------------------------------------------------------------------------------------------------------
Benchmark                                                     Time             CPU   Iterations items_per_second
----------------------------------------------------------------------------------------------------------------
BM_ArithThroughput_T81Cell                              8199941 ns      8194350 ns          326       4.88141M/s Cell vs int64_t (+-*/)
BM_ArithThroughput_T81Cell                              8232245 ns      8221374 ns          326       4.86537M/s Cell vs int64_t (+-*/)
BM_ArithThroughput_T81Cell                              8203562 ns      8196482 ns          326       4.88014M/s Cell vs int64_t (+-*/)
BM_ArithThroughput_T81Cell                              8210455 ns      8199874 ns          326       4.87812M/s Cell vs int64_t (+-*/)
BM_ArithThroughput_T81Cell                              8213317 ns      8206037 ns          326       4.87446M/s Cell vs int64_t (+-*/)
BM_ArithThroughput_T81Cell                              8264631 ns      8252660 ns          326       4.84692M/s Cell vs int64_t (+-*/)
BM_ArithThroughput_T81Cell                              8199492 ns      8192880 ns          326       4.88229M/s Cell vs int64_t (+-*/)
BM_ArithThroughput_T81Cell                              8198741 ns      8192813 ns          326       4.88233M/s Cell vs int64_t (+-*/)
BM_ArithThroughput_T81Cell                              8211616 ns      8204252 ns          326       4.87552M/s Cell vs int64_t (+-*/)
BM_ArithThroughput_T81Cell                              8185289 ns      8180337 ns          326       4.88977M/s Cell vs int64_t (+-*/)
BM_ArithThroughput_T81Cell_mean                         8211929 ns      8204106 ns           10       4.87563M/s Cell vs int64_t (+-*/)
BM_ArithThroughput_T81Cell_median                       8207009 ns      8198178 ns           10       4.87913M/s Cell vs int64_t (+-*/)
BM_ArithThroughput_T81Cell_stddev                         22227 ns        20136 ns           10       11.9262k/s Cell vs int64_t (+-*/)
BM_ArithThroughput_T81Cell_cv                              0.27 %          0.25 %            10            0.24% Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64                                  27251 ns        27231 ns       101467       1.46889G/s Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64                                  27438 ns        27401 ns       101467       1.45978G/s Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64                                  27410 ns        27383 ns       101467       1.46077G/s Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64                                  27270 ns        27246 ns       101467       1.46812G/s Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64                                  27344 ns        27315 ns       101467       1.46439G/s Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64                                  27359 ns        27333 ns       101467       1.46345G/s Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64                                  27353 ns        27319 ns       101467        1.4642G/s Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64                                  27538 ns        27492 ns       101467       1.45496G/s Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64                                  27291 ns        27271 ns       101467       1.46677G/s Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64                                  27235 ns        27221 ns       101467       1.46946G/s Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64_mean                             27349 ns        27321 ns           10       1.46408G/s Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64_median                           27348 ns        27317 ns           10       1.46429G/s Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64_stddev                            94.2 ns         85.6 ns           10       4.58041M/s Cell vs int64_t (+-*/)
BM_ArithThroughput_Int64_cv                                0.34 %          0.31 %            10            0.31% Cell vs int64_t (+-*/)
BM_NegationSpeed_T81Cell                                 106852 ns       106642 ns        26566       937.718M/s Free negation (no borrow)
BM_NegationSpeed_T81Cell                                 105923 ns       105855 ns        26566       944.687M/s Free negation (no borrow)
BM_NegationSpeed_T81Cell                                 106865 ns       106603 ns        26566       938.058M/s Free negation (no borrow)
BM_NegationSpeed_T81Cell                                 107242 ns       106975 ns        26566       934.796M/s Free negation (no borrow)
BM_NegationSpeed_T81Cell                                 106875 ns       106486 ns        26566       939.094M/s Free negation (no borrow)
BM_NegationSpeed_T81Cell                                 105836 ns       105752 ns        26566       945.613M/s Free negation (no borrow)
BM_NegationSpeed_T81Cell                                 106349 ns       106242 ns        26566       941.248M/s Free negation (no borrow)
BM_NegationSpeed_T81Cell                                 106260 ns       105973 ns        26566       943.634M/s Free negation (no borrow)
BM_NegationSpeed_T81Cell                                 105689 ns       105621 ns        26566       946.777M/s Free negation (no borrow)
BM_NegationSpeed_T81Cell                                 106575 ns       106397 ns        26566       939.877M/s Free negation (no borrow)
BM_NegationSpeed_T81Cell_mean                            106447 ns       106255 ns           10        941.15M/s Free negation (no borrow)
BM_NegationSpeed_T81Cell_median                          106462 ns       106319 ns           10       940.562M/s Free negation (no borrow)
BM_NegationSpeed_T81Cell_stddev                             520 ns          442 ns           10       3.91346M/s Free negation (no borrow)
BM_NegationSpeed_T81Cell_cv                                0.49 %          0.42 %            10            0.42% Free negation (no borrow)
➜  docs git:(main) ✗ 

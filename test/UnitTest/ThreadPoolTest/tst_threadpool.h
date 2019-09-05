#ifndef TST_THREADPOOL_H
#define TST_THREADPOOL_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <math.h>
#include <chrono>
#include "../../../src/Util/ThreadPool.h"

using namespace testing;
using namespace std::chrono;

TEST(threadpool, dataset)
{
    RxThreadPool thread_pool(4);
    const size_t loop=1000000;
    std::vector<double> res(loop);
    std::future<double> last_res;

    auto start = system_clock::now();
    std::async([&](){
        for(size_t i=0;i<loop;i++){
            if(i%3==0){
                if(i==loop-1){
                    last_res=thread_pool.post([&,i](){
                        res[i]=std::sqrt((i*60+6666)%250);
                        return res[i];
                     });
                }
                else{
                    thread_pool.post([&,i](){
                       res[i]=std::sqrt((i*60+6666)%250);
                        return res[i];
                    });
                }
            }
        }
    });
    std::async([&](){
        for(size_t i=0;i<loop;i++){
            if(i%3==1){
                if(i==loop-1){
                    last_res=thread_pool.post([&,i](){
                        res[i]=std::sqrt((i*60+6666)%250);
                        return res[i];
                     });
                }
                else{
                    thread_pool.post([&,i](){
                       res[i]=std::sqrt((i*60+6666)%250);
                       return res[i];
                    });
                }
            }
        }
    });
    std::async([&](){
        for(size_t i=0;i<loop;i++){
            if(i%3==2){
                if(i==loop-1){
                    last_res=thread_pool.post([&,i](){
                        res[i]=std::sqrt((i*60+6666)%250);
                        return res[i];
                     });
                }
                else{
                    thread_pool.post([&,i](){
                       res[i]=std::sqrt((i*60+6666)%250);
                       return res[i];
                    });
                }
            }
        }
    });

    last_res.get();
    auto end   = system_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    std::cout <<  "cost "
         << double(duration.count()) * microseconds::period::num / microseconds::period::den
         << " seconds" <<std::endl;

    for(size_t i=0;i<loop;i++){
        auto expect=std::sqrt((i*60+6666)%250);
        auto diff=std::abs(expect-res[i]);
        std::cout<<"i="<<i<<" expect="<<expect<<"  actual="<<res[i]<<"  diff="<<diff<<std::endl;
        EXPECT_EQ(diff,0);
    }


}

#endif // TST_THREADPOOL_H

#ifndef TST_TIMER_H
#define TST_TIMER_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <iostream>
#include <chrono>
#include <cmath>
#include "../../../src/Network/TimerHeap.h"
#include "../../../src/Network/EventLoop.h"



using namespace testing;
using namespace std::chrono;

class TimerTest{
public:
    TimerTest():eventloop(0),timer0(&eventloop),timer1(&eventloop),timer2(&eventloop),timer3(&eventloop){}
    virtual ~TimerTest()=default;
    virtual void timer_cb_1()=0;
    virtual void timer_cb_2()=0;
    virtual void timer_cb_3()=0;
    virtual void timer_cb_4()=0;

    void start_test(){

        timer0.start_timer(1000,[this](){std::cout<<"expired timer "<<timer0.get_id()<<std::endl;
            timer_cb_1(); t0_expired=steady_clock::now();},false);
        timer1.start_timer(2000,[this](){std::cout<<"expired timer "<<timer1.get_id()<<std::endl;
            timer_cb_2(); t1_expired=steady_clock::now();},false);
        timer2.start_timer(3000,[this](){std::cout<<"expired timer "<<timer2.get_id()<<std::endl;
            timer_cb_3(); t2_expired=steady_clock::now();},false);
        timer3.start_timer(4000,[this](){std::cout<<"expired timer "<<timer3.get_id()<<std::endl;
            timer_cb_4(); t3_expired=steady_clock::now();},false);

        t_start=steady_clock::now();
        while (true) {
            usleep(1);
            if(eventloop.get_timer_heap().check_timer_expiry()==0&&eventloop.get_timer_heap().get_timer_num()==0){
                break;
            }
        }
    }
    RxEventLoop eventloop;
    RxTimer timer0;
    RxTimer timer1;
    RxTimer timer2;
    RxTimer timer3;

    steady_clock::time_point t_start;

    steady_clock::time_point t0_expired;
    steady_clock::time_point t1_expired;
    steady_clock::time_point t2_expired;
    steady_clock::time_point t3_expired;
};

class MocTimerTest:public TimerTest{
public:
    MOCK_METHOD0(timer_cb_1,void());
    MOCK_METHOD0(timer_cb_2,void());
    MOCK_METHOD0(timer_cb_3,void());
    MOCK_METHOD0(timer_cb_4,void());
};


TEST(timer_callback_and_accuracy, dataset)
{
    MocTimerTest moc_test;
    moc_test.start_test();
//    EXPECT_CALL(moc_test,timer_cb_1()).Times(1);
//    EXPECT_CALL(moc_test,timer_cb_2()).Times(1);
//    EXPECT_CALL(moc_test,timer_cb_3()).Times(1);
//    EXPECT_CALL(moc_test,timer_cb_4()).Times(1);
    ASSERT_LE(abs(duration_cast<milliseconds>(moc_test.t0_expired-moc_test.t_start).count()-1000),1);
    ASSERT_LE(abs(duration_cast<milliseconds>(moc_test.t1_expired-moc_test.t_start).count()-2000),1);
    ASSERT_LE(abs(duration_cast<milliseconds>(moc_test.t2_expired-moc_test.t_start).count()-3000),1);
    ASSERT_LE(abs(duration_cast<milliseconds>(moc_test.t3_expired-moc_test.t_start).count()-4000),1);
}


TEST(timer_stop,dataset){
    RxEventLoop eventloop(1);
    RxTimer timer0(&eventloop);
    RxTimer timer1(&eventloop);
    RxTimer timer2(&eventloop);
    RxTimer timer3(&eventloop);
    RxTimer timer4(&eventloop);

    RxTimer timer_defer(&eventloop);

    timer0.start_timer(1000,[&](){timer1.stop(); std::cout<<"expired timer "<<timer0.get_id()<<std::endl;},false);
    timer1.start_timer(2000,[&](){std::cout<<"expired timer "<<timer1.get_id()<<std::endl;},false);
    timer2.start_timer(3000,[&](){ timer_defer.start_timer(1000,[](){std::cout<<"defer timer expired"<<std::endl;},false);
        std::cout<<"expired timer "<<timer2.get_id()<<std::endl;},false);

    timer3.start_timer(4000,[&](){std::cout<<"expired timer "<<timer3.get_id()<<std::endl;},false);
    timer4.start_timer(5000,[&](){std::cout<<"expired timer "<<timer4.get_id()<<std::endl;},false);

    while (true) {
        usleep(1);
        if(eventloop.get_timer_heap().check_timer_expiry()==0&&eventloop.get_timer_heap().get_timer_num()==0){
            break;
        }
    }

}

#endif // TST_TIMER_H

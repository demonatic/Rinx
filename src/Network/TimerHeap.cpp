#include "Rinx/Network/TimerHeap.h"
#include <iostream>
#include <assert.h>
#include <thread>
namespace Rinx {

RxTimerHeap::RxTimerHeap():_next_timer_id(0)
{
    heap_init();
}

TimerID RxTimerHeap::add_timer(uint64_t expiry_time,RxTimer *timer)
{
    size_t id=_next_timer_id++;
    _timer_id_map[id]=timer;

    timer->_heap_index=_timer_heap.size();
    timer->_id=id;

    heap_entry entry{expiry_time,timer};
    _timer_heap.emplace_back(entry);

    this->heap_sift_up(timer->_heap_index);

    return id;
}

void RxTimerHeap::remove_timer(TimerID id)
{
    auto it=_timer_id_map.find(id);
    if(it==_timer_id_map.end()){
        return;
    }
    RxTimer *target_timer=it->second;
     _timer_id_map.erase(it);
    remove_timer(target_timer);

}

void RxTimerHeap::remove_timer(RxTimer *timer)
{
    size_t timer_index=timer->_heap_index;

    if(timer_index==get_timer_num()){
        _timer_heap.pop_back();
    }
    else{
        swap_heap_entry(_timer_heap[timer_index],_timer_heap.back());
        _timer_heap.pop_back();
        if(timer_index>1&&_timer_heap[timer_index].expire_time<_timer_heap[timer_index/2].expire_time)
            heap_sift_up(timer_index);
        else
            heap_sift_down(timer_index);
    }
    timer->_heap_index=std::numeric_limits<size_t>::max();
}

int RxTimerHeap::check_timer_expiry()
{
    if(!get_timer_num())
        return 0;

    int expired_num=0;

    for(;;){
        heap_entry *entry=this->get_heap_top();
        if(!entry){
            break;
        }

        uint64_t now=Clock::get_now_tick();
        if(entry->expire_time>now){
            break;
        }

        RxTimer *timer=entry->timer;
        this->pop_heap_top();

        timer->set_active(false);
        timer->expired();
        ++expired_num;

        if(timer->is_repeat()){
            uint64_t new_expire_time=Clock::get_now_tick()+timer->get_duration();
            this->add_timer(new_expire_time,timer);
            timer->set_active(true);
        }
    }
    return expired_num;
}

size_t RxTimerHeap::get_timer_num() const
{
    return _timer_heap.size()-1;
}

uint64_t RxTimerHeap::get_timeout_interval()
{
    uint64_t interval=0,now=Clock::get_now_tick();
    heap_entry *entry=this->get_heap_top();

    if(entry&&entry->expire_time>now){
        interval=entry->expire_time-now;
    }
    return interval;
}

void RxTimerHeap::heap_init()
{
    _timer_heap.reserve(64);
    _timer_heap.push_back({std::numeric_limits<size_t>::max(),nullptr}); //let heap valid starts with index 1
}

void RxTimerHeap::heap_sift_down(size_t index)
{
    while(index*2<_timer_heap.size()){ //not in the last level
        heap_entry &self=_timer_heap[index];
        heap_entry &left_child=_timer_heap[2*index],&right_child=_timer_heap[2*index+1];

        heap_entry &child_min=index*2+1>get_timer_num()||left_child.expire_time<right_child.expire_time?left_child:right_child;
        if(self.expire_time<child_min.expire_time){
            break;
        }

        index=child_min.timer->_heap_index;
        swap_heap_entry(self,child_min);
    }
}

void RxTimerHeap::pop_heap_top()
{
    TimerID removed_id=TimerIDMax;
    if(get_timer_num()==1){
        removed_id=_timer_heap.back().timer->get_id();
       _timer_heap.pop_back();
    }
    else{
        removed_id=get_heap_top()->timer->get_id();
        heap_entry &last_entry=_timer_heap.back();
        swap_heap_entry(*get_heap_top(),last_entry);

        _timer_heap.pop_back();
        heap_sift_down(1);
    }

    assert(removed_id!=TimerIDMax);
    _timer_id_map.erase(removed_id);

}

void RxTimerHeap::heap_sift_up(size_t index)
{
    assert(index<_timer_heap.size());
    while(index>1){
        heap_entry &parent=_timer_heap[index/2];
        heap_entry &self=_timer_heap[index];
        if(self.expire_time>parent.expire_time){
            break;
        }
        swap_heap_entry(parent,self);
        index/=2;
    }
}

RxTimerHeap::heap_entry *RxTimerHeap::get_heap_top()
{
    return _timer_heap.size()==1?nullptr:&_timer_heap[1];
}

void RxTimerHeap::swap_heap_entry(RxTimerHeap::heap_entry &entry_a, RxTimerHeap::heap_entry &entry_b)
{
    std::swap(entry_a,entry_b);
    std::swap(entry_a.timer->_heap_index,entry_b.timer->_heap_index);
}

} //namespace Rinx

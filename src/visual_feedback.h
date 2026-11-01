#pragma once
#include <stdint.h>
namespace nova {
enum FeedbackKind { FeedbackDamage,FeedbackHealing,FeedbackSelfDamage };
struct FeedbackEvent { int32_t x,y;uint32_t frame;uint16_t value;uint8_t kind,active; };
inline FeedbackEvent feedback_events[24];
inline unsigned feedback_next;
inline void resetFeedback(){feedback_next=0;for(unsigned i=0;i<24;++i)feedback_events[i].active=0;}
inline void emitFeedback(FeedbackKind kind,uint32_t frame,int32_t x,int32_t y,unsigned amount){
 if(!amount)return;
 for(unsigned i=0;i<24;++i){FeedbackEvent& e=feedback_events[i];if(e.active&&e.kind==kind&&frame-e.frame<3&&e.x==x&&e.y==y){unsigned sum=e.value+amount;e.value=uint16_t(sum>65535?65535:sum);return;}}
 feedback_events[feedback_next]={x,y,frame,uint16_t(amount>65535?65535:amount),uint8_t(kind),1};feedback_next=(feedback_next+1)%24;
}
}

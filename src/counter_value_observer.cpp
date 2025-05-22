#include <avt_vimba_camera/counter_value_observer.h>
#include <iostream>

CounterValueObserver::CounterValueObserver(Callback callback)
  : callback_(callback)
{
}

void CounterValueObserver::FeatureChanged(const FeaturePtr& vimba_feature_ptr)
{
    callback_(vimba_feature_ptr);
}

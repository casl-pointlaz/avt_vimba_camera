#ifndef COUNTER_VALUE_OBSERVER_H
#define COUNTER_VALUE_OBSERVER_H

#include <VimbaCPP/Include/VimbaCPP.h>

#include <functional>

using namespace AVT::VmbAPI;

class CounterValueObserver : virtual public IFeatureObserver
{
public:
  typedef std::function<void(const FeaturePtr &vimba_feature_ptr)> Callback;

  CounterValueObserver(Callback callback);

  // Destructor
  ~CounterValueObserver(){};

  // This is our callback routine that will be executed on every feature change
  virtual void FeatureChanged( const FeaturePtr &vimba_feature_ptr );

private:
  Callback callback_;
};

#endif

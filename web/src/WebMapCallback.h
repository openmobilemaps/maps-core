#include "MapCallbackInterface.h"
#include "WebMapCallbackInterface.h"

#include <atomic>
#include <memory>

//! Invalidation callback for MapInterface / MapScene.
class WebMapCallback : public WebMapCallbackInterface, public MapCallbackInterface, public std::enable_shared_from_this<WebMapCallback> {
  public:
    WebMapCallback(): invalid(true) {}

    virtual void invalidate() override;

    virtual void onMapResumed() override;

    virtual bool getAndResetInvalid() override;

    virtual std::shared_ptr<MapCallbackInterface> asCallbackInterface() override;

  private:
    std::atomic_bool invalid;
};

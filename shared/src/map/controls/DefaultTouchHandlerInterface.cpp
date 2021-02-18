#include "DefaultTouchHandlerInterface.h"
#include "DefaultTouchHandler.h"

std::shared_ptr<TouchHandlerInterface> DefaultTouchHandlerInterface::create(const std::shared_ptr<::SchedulerInterface> &scheduler,
                                                                            float density) {
    return std::make_shared<DefaultTouchHandler>(scheduler, density);
}

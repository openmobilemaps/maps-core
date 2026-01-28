#import <UIKit/UIKit.h>
@import MapCoreSharedModule;

NS_ASSUME_NONNULL_BEGIN

@interface MCMapViewObjC : UIView

- (instancetype)initWithFrame:(CGRect)frame NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithCoder:(NSCoder *)coder NS_DESIGNATED_INITIALIZER;

@property (nonatomic, readonly) UIView *mapView;
@property (nonatomic, readonly) MCMapInterface *mapInterface;

@end

NS_ASSUME_NONNULL_END

#import "MCMapViewObjC.h"

@import MapCore;
@import MapCoreSharedModule;

@interface MCMapViewObjC ()
@property (nonatomic, strong) MCMapView *mapViewInternal;
@end

@implementation MCMapViewObjC

- (instancetype)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        [self commonInit];
    }
    return self;
}

- (instancetype)initWithCoder:(NSCoder *)coder {
    if (self = [super initWithCoder:coder]) {
        [self commonInit];
    }
    return self;
}

- (UIView *)mapView {
    return self.mapViewInternal;
}

- (MCMapInterface *)mapInterface {
    return self.mapViewInternal.mapInterface;
}

- (void)layoutSubviews {
    [super layoutSubviews];
    self.mapViewInternal.frame = self.bounds;
}

- (void)commonInit {
    MCMapConfig *config = [[MCMapConfig alloc] initWithMapCoordinateSystem:[MCCoordinateSystemFactory getEpsg3857System]];
    self.mapViewInternal = [[MCMapView alloc] initWithMapConfig:config pixelsPerInch:nil is3D:NO];
    self.mapViewInternal.frame = self.bounds;
    self.mapViewInternal.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self addSubview:self.mapViewInternal];
}

@end

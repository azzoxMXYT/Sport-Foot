#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import "game.h"

// Both buffers use the engine's little-endian 0x00RRGGBB pixel layout.
static CGContextRef Bitmap(void *data, size_t width, size_t height) {
    CGColorSpaceRef color = CGColorSpaceCreateDeviceRGB();
    CGContextRef result = CGBitmapContextCreate(data, width, height, 8, width * 4, color,
                                kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst);
    CGColorSpaceRelease(color);
    return result;
}

@interface PitchView : UIView
- (void)setRunning:(BOOL)running;
@end

@implementation PitchView {
    CADisplayLink *_link;
    CFTimeInterval _previous;
    CGColorSpaceRef _color;
    __weak UITouch *_stick;
    CGPoint _anchor;
}
- (instancetype)initWithFrame:(CGRect)frame {
    if ((self = [super initWithFrame:frame])) {
        self.multipleTouchEnabled = YES;
        self.opaque = YES;
        self.layer.magnificationFilter = kCAFilterNearest;
        _color = CGColorSpaceCreateDeviceRGB();
        CGImageRef cover = [UIImage imageNamed:@"cover.png"].CGImage;
        CGContextRef bitmap = Bitmap(sfCover, 408, 411);
        if (cover && bitmap) CGContextDrawImage(bitmap, CGRectMake(0,0,408,411), cover);
        if (bitmap) CGContextRelease(bitmap);
        mobile = 1;
        init();
        [self renderFrame];
    }
    return self;
}
- (void)dealloc { CGColorSpaceRelease(_color); }
- (void)renderFrame {
    draw();
    // Core Animation may still display this frame after the next tick.
    NSData *snapshot = [NSData dataWithBytes:pixels length:sizeof(pixels)];
    CGDataProviderRef provider = CGDataProviderCreateWithCFData((__bridge CFDataRef)snapshot);
    CGImageRef image = CGImageCreate(W,H,8,32,W*4,_color,
            kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst,provider,NULL,false,kCGRenderingIntentDefault);
    self.layer.contents = (__bridge id)image;
    CGImageRelease(image);
    CGDataProviderRelease(provider);
}
- (void)releaseStick { _stick = nil; axisX = axisY = 0; sprint = 0; }
- (void)setRunning:(BOOL)running {
    if (!running) {
        [_link invalidate]; _link = nil; _previous = 0;
        [self releaseStick];
        if (mode == PLAY) action(BACK);
    } else if (!_link) {
        _link = [CADisplayLink displayLinkWithTarget:self selector:@selector(step:)];
        _link.preferredFramesPerSecond = 60;
        [_link addToRunLoop:NSRunLoop.mainRunLoop forMode:NSRunLoopCommonModes];
    }
}
- (void)step:(CADisplayLink *)link {
    tick(_previous ? (float)(link.timestamp - _previous) : 0);
    _previous = link.timestamp;
    [self renderFrame];
}
- (CGPoint)logical:(UITouch *)touch {
    CGPoint p = [touch locationInView:self];
    return CGPointMake(p.x * W / self.bounds.size.width, p.y * H / self.bounds.size.height);
}
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    for (UITouch *touch in touches) {
        CGPoint p = [self logical:touch];
        if (mode == PLAY && p.x < W/2 && !_stick) { _stick = touch; _anchor = p; }
        else { tap((int)p.x,(int)p.y); if (mode != PLAY) [self releaseStick]; }
    }
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    if (!_stick || ![touches containsObject:_stick] || mode != PLAY) return;
    CGPoint p = [self logical:_stick];
    axisX = (p.x - _anchor.x)/26; axisY = (p.y - _anchor.y)/26;
    sprint = axisX * axisX + axisY * axisY > 1.3f;
}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    if (_stick && [touches containsObject:_stick]) [self releaseStick];
}
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
    [self touchesEnded:touches withEvent:event];
}
@end

@interface MatchController : UIViewController
@property(nonatomic,strong) PitchView *pitch;
@end
@implementation MatchController
- (void)viewDidLoad {
    [super viewDidLoad]; self.view.backgroundColor = UIColor.blackColor;
    self.pitch = [[PitchView alloc] initWithFrame:CGRectZero]; [self.view addSubview:self.pitch];
}
- (void)viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    CGRect safe = self.view.safeAreaLayoutGuide.layoutFrame;
    CGFloat scale = MIN(safe.size.width/W,safe.size.height/H);
    self.pitch.frame = CGRectMake(CGRectGetMidX(safe)-W*scale/2,CGRectGetMidY(safe)-H*scale/2,W*scale,H*scale);
}
- (BOOL)prefersStatusBarHidden { return YES; }
- (UIInterfaceOrientationMask)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscape; }
@end

@interface GameScene : UIResponder <UIWindowSceneDelegate>
@property(nonatomic,strong) UIWindow *window;
@property(nonatomic,strong) MatchController *match;
@end
@implementation GameScene
- (void)scene:(UIScene *)scene willConnectToSession:(UISceneSession *)session options:(UISceneConnectionOptions *)options {
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
    self.match = [MatchController new]; self.window.rootViewController = self.match;
    [self.window makeKeyAndVisible];
}
- (void)sceneDidBecomeActive:(UIScene *)scene { [self.match loadViewIfNeeded]; [self.match.pitch setRunning:YES]; }
- (void)sceneWillResignActive:(UIScene *)scene { [self.match.pitch setRunning:NO]; }
- (void)sceneDidDisconnect:(UIScene *)scene { [self.match.pitch setRunning:NO]; }
@end
@interface AppDelegate : UIResponder <UIApplicationDelegate>
@end
@implementation AppDelegate
@end
int main(int argc, char **argv) {
    @autoreleasepool { return UIApplicationMain(argc,argv,nil,NSStringFromClass(AppDelegate.class)); }
}

#import <UIKit/UIKit.h>

@class MWMBasePlacePageView, MWMPlacePageViewManager, MWMPlacePageActionBar;

@interface MWMPlacePage : NSObject

@property (nonatomic) IBOutlet MWMBasePlacePageView * basePlacePageView;
@property (nonatomic) IBOutlet UIView * extendedPlacePageView;
@property (nonatomic) IBOutlet UIImageView * anchorImageView;
@property (nonatomic) IBOutlet UIPanGestureRecognizer * panRecognizer;
@property (weak, nonatomic, readonly) MWMPlacePageViewManager * manager;
@property (nonatomic) MWMPlacePageActionBar * actionBar;
@property (nonatomic) CGFloat topBound;
@property (nonatomic) CGFloat leftBound;
@property (nonatomic) CGFloat parentViewHeight;
@property (nonatomic) CGFloat keyboardHeight;

- (instancetype)initWithManager:(MWMPlacePageViewManager *)manager;
- (void)show;
- (void)hide;
- (void)dismiss;
- (void)configure;

#pragma mark - Actions
- (void)addBookmark;
- (void)removeBookmark;
- (void)editBookmark;
- (void)editPlace;
- (void)addBusiness;
- (void)addPlace;
- (void)reloadBookmark;
- (void)addPlacePageShadowToView:(UIView *)view offset:(CGSize)offset;

- (IBAction)didTap:(UITapGestureRecognizer *)sender;

- (void)setDirectionArrowTransform:(CGAffineTransform)transform;
- (void)setDistance:(NSString *)distance;
- (void)updateMyPositionStatus:(NSString *)status;

- (void)refresh;

- (instancetype)init __attribute__((unavailable("call initWithManager: instead")));

@end

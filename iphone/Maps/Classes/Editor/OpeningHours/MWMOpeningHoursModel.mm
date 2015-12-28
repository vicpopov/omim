#import "MWMOpeningHoursCommon.h"
#import "MWMOpeningHoursModel.h"
#import "MWMOpeningHoursSection.h"

#include "editor/opening_hours_ui.hpp"
#include "editor/ui2oh.hpp"

extern UITableViewRowAnimation const kMWMOpeningHoursEditorRowAnimation = UITableViewRowAnimationFade;

@interface MWMOpeningHoursModel () <MWMOpeningHoursSectionProtocol>

@property (weak, nonatomic) id<MWMOpeningHoursModelProtocol> delegate;

@property (nonatomic) NSMutableArray<MWMOpeningHoursSection *> * sections;

@end

using namespace editor;
using namespace osmoh;

@implementation MWMOpeningHoursModel
{
  ui::TimeTableSet timeTableSet;
}

- (instancetype _Nullable)initWithDelegate:(id<MWMOpeningHoursModelProtocol> _Nonnull)delegate
{
  self = [super init];
  if (self)
  {
    self.delegate = delegate;
    self.isSimpleMode = self.isSimpleModeCapable;
  }
  return self;
}

- (void)addSection
{
  [self.sections addObject:[[MWMOpeningHoursSection alloc] initWithDelegate:self]];
  [self refreshSectionsIndexes];
}

- (void)refreshSectionsIndexes
{
  [self.sections enumerateObjectsUsingBlock:^(MWMOpeningHoursSection * _Nonnull section,
                                              NSUInteger idx, BOOL * _Nonnull stop)
  {
    [section refreshIndex:idx];
  }];
}

- (void)addSchedule
{
  NSAssert(self.canAddSection, @"Can not add schedule");
  timeTableSet.Append(timeTableSet.GetComplementTimeTable());
  [self addSection];
  [self.tableView reloadSections:[[NSIndexSet alloc] initWithIndex:self.sections.count - 1]
                withRowAnimation:kMWMOpeningHoursEditorRowAnimation];
  NSAssert(timeTableSet.Size() == self.sections.count, @"Inconsistent state");
  [self.sections[self.sections.count - 1] scrollIntoView];
}

- (void)deleteSchedule:(NSUInteger)index
{
  NSAssert(index < self.count, @"Invalid section index");
  BOOL const needRealDelete = self.canAddSection;
  timeTableSet.Remove(index);
  [self.sections removeObjectAtIndex:index];
  [self refreshSectionsIndexes];
  if (needRealDelete)
  {
    [self.tableView deleteSections:[[NSIndexSet alloc] initWithIndex:index]
                  withRowAnimation:kMWMOpeningHoursEditorRowAnimation];
    [self.tableView reloadSections:[[NSIndexSet alloc] initWithIndex:self.count]
                  withRowAnimation:kMWMOpeningHoursEditorRowAnimation];
  }
  else
  {
    NSRange reloadRange = {index, self.count - index + 1};
    [self.tableView reloadSections:[[NSIndexSet alloc] initWithIndexesInRange:reloadRange]
                  withRowAnimation:kMWMOpeningHoursEditorRowAnimation];
  }
}

- (void)updateActiveSection:(NSUInteger)index
{
  for (MWMOpeningHoursSection * section in self.sections)
  {
    if (section.index != index)
      section.selectedRow = nil;
  }
}

- (ui::TTimeTableProxy)getTimeTableProxy:(NSUInteger)index
{
  NSAssert(index < self.count, @"Invalid section index");
  return timeTableSet.Get(index);
}

- (MWMOpeningHoursEditorCells)cellKeyForIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSUInteger const section = indexPath.section;
  NSAssert(section < self.count, @"Invalid section index");
  return [self.sections[section] cellKeyForRow:indexPath.row];
}

- (CGFloat)heightForIndexPath:(NSIndexPath * _Nonnull)indexPath withWidth:(CGFloat)width
{
  NSUInteger const section = indexPath.section;
  NSAssert(section < self.count, @"Invalid section index");
  return [self.sections[section] heightForRow:indexPath.row withWidth:width];
}

- (void)fillCell:(MWMOpeningHoursTableViewCell * _Nonnull)cell atIndexPath:(NSIndexPath * _Nonnull)indexPath
{
  NSUInteger const section = indexPath.section;
  NSAssert(section < self.count, @"Invalid section index");
  cell.indexPathAtInit = indexPath;
  cell.section = self.sections[section];
}

- (NSUInteger)numberOfRowsInSection:(NSUInteger)section
{
  NSAssert(section < self.count, @"Invalid section index");
  return self.sections[section].numberOfRows;
}

- (ui::TOpeningDays)unhandledDays
{
  return timeTableSet.GetUnhandledDays();
}

- (void)updateOpeningHours
{
  stringstream sstr;
  sstr << MakeOpeningHours(timeTableSet).GetRule();
  self.delegate.openingHours = @(sstr.str().c_str());
}

#pragma mark - Properties

- (NSUInteger)count
{
  NSAssert(timeTableSet.Size() == self.sections.count, @"Inconsistent state");
  return self.sections.count;
}

- (BOOL)canAddSection
{
  return !timeTableSet.GetUnhandledDays().empty();
}

- (UITableView *)tableView
{
  return self.delegate.tableView;
}

- (BOOL)isValid
{
  return osmoh::OpeningHours(self.delegate.openingHours.UTF8String).IsValid();
}

- (void)setIsSimpleMode:(BOOL)isSimpleMode
{
  if (_isSimpleMode == isSimpleMode)
    return;
  _isSimpleMode = isSimpleMode;
  id<MWMOpeningHoursModelProtocol> delegate = self.delegate;
  if (isSimpleMode && MakeTimeTableSet(osmoh::OpeningHours(delegate.openingHours.UTF8String), timeTableSet))
  {
    _isSimpleMode = YES;
    delegate.tableView.hidden = NO;
    delegate.advancedEditor.hidden = YES;
    [delegate.toggleModeButton setTitle:L(@"advanced_mode") forState:UIControlStateNormal];
    _sections = [NSMutableArray arrayWithCapacity:timeTableSet.Size()];
    while (self.sections.count < timeTableSet.Size())
      [self addSection];
    [delegate.tableView reloadData];
  }
  else
  {
    _isSimpleMode = NO;
    [self updateOpeningHours];
    delegate.tableView.hidden = YES;
    delegate.advancedEditor.hidden = NO;
    [delegate.toggleModeButton setTitle:L(@"simple_mode") forState:UIControlStateNormal];
    MWMTextView * ev = delegate.editorView;
    ev.text = delegate.openingHours;
    [ev becomeFirstResponder];
  }
}

- (BOOL)isSimpleModeCapable
{
  ui::TimeTableSet tts;
  return MakeTimeTableSet(osmoh::OpeningHours(self.delegate.openingHours.UTF8String), tts);
}

@end

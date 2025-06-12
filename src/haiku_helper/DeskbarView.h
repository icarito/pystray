#ifndef DESKBAR_VIEW_H
#define DESKBAR_VIEW_H

#include <View.h>
#include <Point.h> // For BPoint

// Name of the replicant, must match what Deskbar uses and what's in the Makefile/resources
#define DESKBAR_VIEW_NAME "PystrayDeskbarView"

class DeskbarView : public BView {
public:
    DeskbarView(BRect frame); // Constructor for adding to a window
    DeskbarView(BMessage* archive); // Constructor for unarchiving (instantiated by Deskbar)
    virtual ~DeskbarView();

    // BView overrides
    virtual void AttachedToWindow();
    virtual void DetachedFromWindow();
    virtual void Draw(BRect updateRect);
    virtual void MouseDown(BPoint point);
    virtual void MessageReceived(BMessage* message);

    // Archiving mechanism, required for replicants
    static DeskbarView* Instantiate(BMessage* archive);
    virtual status_t Archive(BMessage* archive, bool deep = true) const;

private:
    // Add any specific members for your view, e.g., icon, state
    // BBitmap* icon;
};

#endif // DESKBAR_VIEW_H

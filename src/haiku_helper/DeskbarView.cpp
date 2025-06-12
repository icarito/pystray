#include "DeskbarView.h"
#include "App.h" // For APP_SIGNATURE

#include <Alert.h>
#include <Application.h> // For be_app_messenger
#include <Bitmap.h>      // For BBitmap (icon)
#include <Deskbar.h>     // For BDeskbar
#include <MenuItem.h>    // For context menu
#include <PopUpMenu.h>   // For context menu
#include <Roster.h>      // For app_info, etc.
#include <stdio.h>       // For printf
#include <string.h>      // For strerror, isprint (for msg_what_to_string_dv)
#include <TranslationUtils.h> // For BTranslationUtils for loading icons

// Helper function (similar to one in App.cpp and MainWindow.cpp)
// Ensure this helper is robust for various 'what' values.
static void msg_what_to_string_dv(uint32 what, char* buffer, size_t buffer_size) {
    // Try to interpret as 4-char code
    char c1 = (what >> 24) & 0xFF;
    char c2 = (what >> 16) & 0xFF;
    char c3 = (what >> 8) & 0xFF;
    char c4 = what & 0xFF;

    if (isprint(c1) && isprint(c2) && isprint(c3) && isprint(c4)) {
        snprintf(buffer, buffer_size, "'%c%c%c%c'", c1, c2, c3, c4);
    } else {
        // Fallback to hex if not all chars are printable
        snprintf(buffer, buffer_size, "0x%08x", (unsigned int)what);
    }
}


// Constructor for direct instantiation
DeskbarView::DeskbarView(BRect frame)
    : BView(frame, DESKBAR_VIEW_NAME, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE),
      fIconBitmap(NULL) { // Initialize fIconBitmap
    mIconRef.name = NULL; // Initialize mIconRef as per your .h change

    printf("DeskbarView: Constructor(BRect) - Name: '%s', Frame: L:%.1f, T:%.1f, R:%.1f, B:%.1f\n",
           Name(), frame.left, frame.top, frame.right, frame.bottom);
    printf("DeskbarView: Constructor(BRect) - ResizingMode: 0x%X, Flags: 0x%X\n", ResizingMode(), Flags());

    SetExplicitMinSize(BSize(15.0, 15.0));
    SetExplicitMaxSize(BSize(15.0, 15.0));
    SetExplicitPreferredSize(BSize(15.0, 15.0));
    // SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR)); // Done in AttachedToWindow
    printf("DeskbarView: Constructor(BRect) finished.\n");
}

// Constructor for Deskbar instantiation (unarchiving)
DeskbarView::DeskbarView(BMessage* archive)
    : BView(archive), // CRITICAL: Call parent's BMessage constructor
      fIconBitmap(NULL) { // Initialize fIconBitmap
    mIconRef.name = NULL; // Initialize mIconRef

    printf("DeskbarView: Constructor(BMessage archive) - Unarchiving view.\n");
    // archive->PrintToStream(); // For very verbose debugging of archive contents

    SetExplicitMinSize(BSize(15.0, 15.0));
    SetExplicitMaxSize(BSize(15.0, 15.0));
    SetExplicitPreferredSize(BSize(15.0, 15.0));

    if (archive->FindRef("icon_ref", &mIconRef) == B_OK && mIconRef.name != NULL) {
        fIconBitmap = BTranslationUtils::GetBitmap(&mIconRef);
        if (fIconBitmap) {
            printf("DeskbarView: Constructor(BMessage) - Icon loaded from icon_ref in archive (name: %s).\n", mIconRef.name);
        } else {
            fprintf(stderr, "DeskbarView: Constructor(BMessage) - ERROR: Failed to load icon from icon_ref in archive (name: %s, dev: %lld, dir: %lld).\n",
                mIconRef.name, mIconRef.device, mIconRef.directory);
        }
    } else {
        printf("DeskbarView: Constructor(BMessage) - No valid icon_ref found in archive.\n");
    }
    if (!fIconBitmap) {
        printf("DeskbarView: Constructor(BMessage) - fIconBitmap is NULL after archive processing.\n");
        // Optionally load a default icon here if desired
    }
    printf("DeskbarView: Constructor(BMessage archive) finished.\n");
}

DeskbarView::~DeskbarView() {
    printf("DeskbarView: Destructor called.\n");
    delete fIconBitmap;
    fIconBitmap = NULL;
}

// Required static function for Deskbar to instantiate the replicant
DeskbarView* DeskbarView::Instantiate(BMessage* archive) {
    printf("DeskbarView: Instantiate called.\n");
    if (!validate_instantiation(archive, "DeskbarView")) {
        fprintf(stderr, "DeskbarView: Instantiate - ERROR: Validation failed.\n");
        return NULL;
    }
    printf("DeskbarView: Instantiate successful, creating new DeskbarView from archive.\n");
    return new DeskbarView(archive);
}

// Required for replicants to save their state
status_t DeskbarView::Archive(BMessage* archive, bool deep) const {
    printf("DeskbarView: Archive called (deep: %s).\n", deep ? "true" : "false");
    status_t status = BView::Archive(archive, deep);
    if (status != B_OK) {
        fprintf(stderr, "DeskbarView: Archive - ERROR from BView::Archive: %s\n", strerror(status));
        return status;
    }

    // APP_SIGNATURE should be defined in App.h (extern const char*) and App.cpp
    extern const char* APP_SIGNATURE;
    status = archive->AddString("add_on", APP_SIGNATURE);
    if (status != B_OK) {
        fprintf(stderr, "DeskbarView: Archive - ERROR adding 'add_on' (%s): %s\n", APP_SIGNATURE, strerror(status));
        // Not returning status here to allow class to be added.
    }

    status = archive->AddString("class", "DeskbarView");
    if (status != B_OK) {
        fprintf(stderr, "DeskbarView: Archive - ERROR adding 'class' (DeskbarView): %s\n", strerror(status));
        // Not returning status here.
    }

    if (mIconRef.name != NULL && mIconRef.device != 0) {
        status = archive->AddRef("icon_ref", &mIconRef);
        if (status == B_OK) {
            printf("DeskbarView: Archive - Archived icon_ref (name: %s).\n", mIconRef.name);
        } else {
            fprintf(stderr, "DeskbarView: Archive - ERROR adding 'icon_ref': %s\n", strerror(status));
        }
    } else {
        printf("DeskbarView: Archive - mIconRef is not set or invalid, not archiving icon_ref.\n");
    }

    printf("DeskbarView: Archive finished.\n");
    // archive->PrintToStream(); // For debugging archived content
    return status; // Return the last significant status or B_OK if minor errors were ignored
}

void DeskbarView::AttachedToWindow() {
    BView::AttachedToWindow();
    printf("DeskbarView: AttachedToWindow called.\n");

    // Set a bright, obvious color for debugging if the icon doesn't draw
    SetViewColor(255, 0, 0, 255); // Bright Red for debugging visibility

    if (Parent()) {
        // SetViewColor(Parent()->ViewColor()); // This would make it transparent to Deskbar bg immediately
        printf("DeskbarView: AttachedToWindow - Parent view exists. Background will be set in Draw().\n");
    } else {
        printf("DeskbarView: AttachedToWindow - No parent view. Will use its own ViewColor for background.\n");
    }

    // Ensure explicit size is set, common for Deskbar icons
    SetExplicitMinSize(BSize(15.0,15.0));
    SetExplicitMaxSize(BSize(15.0,15.0));
    SetExplicitPreferredSize(BSize(15.0,15.0));
    // SetExplicitAlignment(BAlignment(B_ALIGN_CENTER, B_ALIGN_MIDDLE)); // Already default

    BRect bounds = Bounds();
    printf("DeskbarView: AttachedToWindow - Current Bounds: L:%.1f, T:%.1f, R:%.1f, B:%.1f\n",
           bounds.left, bounds.top, bounds.right, bounds.bottom);
    // ResizeTo(15.0, 15.0); // Should not be needed if preferred/min/max are set
    printf("DeskbarView: AttachedToWindow finished.\n");
}

void DeskbarView::DetachedFromWindow() {
    printf("DeskbarView: DetachedFromWindow called.\n");
    BView::DetachedFromWindow();
}

void DeskbarView::GetPreferredSize(float* _width, float* _height) {
    // The input values might be uninitialized or carry values from a previous call.
    // printf("DeskbarView: GetPreferredSize called. Current input values: width=%.1f, height=%.1f\n", *_width, *_height);
    *_width = 15.0;
    *_height = 15.0;
    printf("DeskbarView: GetPreferredSize - Setting preferred size: width=%.1f, height=%.1f\n", *_width, *_height);
}


void DeskbarView::Draw(BRect updateRect) {
    // BView::Draw(updateRect); // This fills with ViewColor. We'll do it conditionally.
    printf("DeskbarView: Draw() called. UpdateRect: L:%.1f, T:%.1f, R:%.1f, B:%.1f\n",
           updateRect.left, updateRect.top, updateRect.right, updateRect.bottom);

    BRect bounds = Bounds();
    printf("DeskbarView: Draw - Current Bounds: L:%.1f, T:%.1f, R:%.1f, B:%.1f\n",
           bounds.left, bounds.top, bounds.right, bounds.bottom);

    // Match Deskbar background color for transparency
    if (Parent()) {
        SetLowColor(Parent()->ViewColor()); // Set low color to parent's view color
        FillRect(bounds, B_SOLID_LOW);      // Fill with this low color (i.e., transparent to parent)
        printf("DeskbarView: Draw - Filled with Parent's ViewColor.\n");
    } else {
         SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
         FillRect(bounds, B_SOLID_LOW);
         printf("DeskbarView: Draw - No Parent, filled with B_PANEL_BACKGROUND_COLOR.\n");
    }

    if (fIconBitmap && fIconBitmap->IsValid()) {
        printf("DeskbarView: Draw - fIconBitmap exists and is valid. Attempting to draw it.\n");
        SetDrawingMode(B_OP_ALPHA);
        // Draw bitmap at (0,0) relative to this view's bounds.
        // Ensure the bitmap is the correct size (e.g. 16x16) or scale it.
        DrawBitmap(fIconBitmap, BRect(0,0,15,15)); // Draw into 16x16 bounds
        SetDrawingMode(B_OP_COPY);
        printf("DeskbarView: Draw - Drew fIconBitmap.\n");
    } else {
        printf("DeskbarView: Draw - fIconBitmap is NULL or invalid. Drawing fallback green square.\n");
        SetHighColor(0, 255, 0, 255); // Bright Green
        FillRect(bounds);
        // Optional: Draw a small border to see view extents clearly
        SetHighColor(0,0,0); // Black border
        StrokeRect(bounds);
        printf("DeskbarView: Draw - Fallback green square drawn.\n");
    }
    printf("DeskbarView: Draw - Drawing finished.\n");
}

void DeskbarView::MessageReceived(BMessage* message) {
    char whatStr[12]; // Increased buffer size for hex output
    msg_what_to_string_dv(message->what, whatStr, sizeof(whatStr));
    printf("DeskbarView: MessageReceived - what: %s\n", whatStr);
    // message->PrintToStream(); // Very verbose, enable if needed for specific messages

    switch (message->what) {
        case 'VICN': // View Icon Change
        {
            const char* path = NULL;
            if (message->FindString("icon_path", &path) == B_OK && path) {
                printf("DeskbarView: Message 'VICN' received. Path: '%s'. Action: Attempting to load new icon.\n", path);
                delete fIconBitmap;
                fIconBitmap = BTranslationUtils::GetBitmapFile(path);
                if (fIconBitmap && fIconBitmap->IsValid()) {
                    printf("DeskbarView: 'VICN' - Successfully loaded new icon from '%s'. Invalidating view.\n", path);
                    // Optionally update mIconRef here if this path should be persisted
                    // get_ref_for_path(path, &mIconRef);
                } else {
                    fprintf(stderr, "DeskbarView: 'VICN' - ERROR: Failed to load icon from '%s'. Bitmap is NULL or invalid.\n", path);
                    delete fIconBitmap;
                    fIconBitmap = NULL;
                }
                Invalidate();
            } else {
                 fprintf(stderr, "DeskbarView: Message 'VICN' - ERROR: Could not find 'icon_path' string or path is NULL.\n");
            }
            break;
        }
        case 'VTIL': // View Title Change
        {
            const char* new_title = NULL;
            if (message->FindString("title", &new_title) == B_OK) {
                printf("DeskbarView: Message 'VTIL' received. Title: '%s'. Action: Setting tooltip.\n", new_title ? new_title : "NULL");
                SetToolTip(new_title ? new_title : "");
            } else {
                fprintf(stderr, "DeskbarView: Message 'VTIL' - ERROR: Could not find 'title' string.\n");
            }
            break;
        }
        default:
            printf("DeskbarView: Message unhandled by DeskbarView (what: %s), passing to BView::MessageReceived.\n", whatStr);
            BView::MessageReceived(message);
            break;
    }
    printf("DeskbarView: MessageReceived - Finished processing what: '%s'.\n", whatStr);
}


void DeskbarView::MouseDown(BPoint point) {
    printf("DeskbarView: MouseDown at (X:%.1f, Y:%.1f)\n", point.x, point.y);
    BMessage* currentMsg = Window()->CurrentMessage();
    if (!currentMsg) {
        fprintf(stderr, "DeskbarView: MouseDown - ERROR: Window()->CurrentMessage() is NULL.\n");
        return;
    }
    uint32 buttons = 0;
// No change, content is identical
    if (currentMsg->FindInt32("buttons", (int32*)&buttons) != B_OK) {
        fprintf(stderr, "DeskbarView: MouseDown - ERROR: Could not get 'buttons' from mouse message.\n");
        return;
    }
    printf("DeskbarView: MouseDown - Buttons: 0x%X\n", buttons);

    if (buttons & B_PRIMARY_MOUSE_BUTTON) {
        printf("DeskbarView: MouseDown - Primary button clicked. Action: Showing alert.\n");
        (new BAlert("Pystray", "Pystray Helper Left Clicked!", "OK"))->Go(NULL);
    } else if (buttons & B_SECONDARY_MOUSE_BUTTON) {
        printf("DeskbarView: MouseDown - Secondary button clicked. Action: Showing context menu.\n");
        BPopUpMenu* menu = new BPopUpMenu("DeskbarViewMenu", false, false);
        menu->AddItem(new BMenuItem("About Pystray Helper...", new BMessage('ABUT')));
        menu->AddSeparatorItem();
        menu->AddItem(new BMenuItem("Item 1 (Placeholder)", new BMessage('ITM1')));
        menu->AddItem(new BMenuItem("Item 2 (Placeholder)", new BMessage('ITM2')));
        menu->AddSeparatorItem();
        menu->AddItem(new BMenuItem("Quit Helper", new BMessage(B_QUIT_REQUESTED)));

        menu->SetTargetForItems(be_app_messenger);

        BPoint screenPoint = ConvertToScreen(point);
        printf("DeskbarView: MouseDown - Showing menu at screen coordinates (X:%.1f, Y:%.1f).\n", screenPoint.x, screenPoint.y);
        menu->Go(screenPoint, true, true, true);
    }
}

extern "C" _EXPORT BView* instantiate_deskbar_item(void) {
    printf("DeskbarView: instantiate_deskbar_item() C function called by Deskbar.\n");
    BRect frame(0, 0, 15, 15);
    printf("DeskbarView: instantiate_deskbar_item - Creating DeskbarView instance with frame L:%.1f, T:%.1f, R:%.1f, B:%.1f.\n",
           frame.left, frame.top, frame.right, frame.bottom);
    return new DeskbarView(frame);
}

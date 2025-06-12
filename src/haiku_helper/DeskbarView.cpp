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


// Constructor for direct instantiation (e.g., in MainWindow for testing)
DeskbarView::DeskbarView(BRect frame)
    : BView(frame, DESKBAR_VIEW_NAME, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_NAVIGABLE) {
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR)); // Match Deskbar appearance
    // In a real app, load an icon here
    // icon = ...;
    printf("DeskbarView constructor (BRect)\n");
}

// Constructor for Deskbar instantiation (unarchiving)
DeskbarView::DeskbarView(BMessage* archive)
    : BView(archive) {
    // icon = NULL; // Initialize members
    // status_t status = archive->FindPointer("icon_ptr", (void**)&icon); // Example
    printf("DeskbarView constructor (BMessage archive)\n");
}

DeskbarView::~DeskbarView() {
    // delete icon; // Clean up resources
    printf("DeskbarView destructor\n");
}

// Required static function for Deskbar to instantiate the replicant
DeskbarView* DeskbarView::Instantiate(BMessage* archive) {
    if (!validate_instantiation(archive, "DeskbarView")) {
        printf("DeskbarView::Instantiate validation failed\n");
        return NULL;
    }
    printf("DeskbarView::Instantiate successful\n");
    return new DeskbarView(archive);
}

// Required for replicants to save their state
status_t DeskbarView::Archive(BMessage* archive, bool deep) const {
    status_t status = BView::Archive(archive, deep);
    if (status != B_OK) return status;

    // Add our class name and the application signature (required for Deskbar)
    status = archive->AddString("add_on", APP_SIGNATURE);
    if (status != B_OK) return status;

    status = archive->AddString("class", "DeskbarView"); // Make sure this matches the class name
    // if (icon) archive->AddPointer("icon_ptr", icon); // Example
    printf("DeskbarView::Archive status: %s\n", strerror(status));
    return status;
}

void DeskbarView::AttachedToWindow() {
    BView::AttachedToWindow();
    printf("DeskbarView::AttachedToWindow\n");
    if (Parent()) {
        SetViewColor(Parent()->ViewColor()); // Match parent view color (Deskbar tray)
    }
    // If you have an icon, you might want to resize the view to fit it
    // if (icon) SetExplicitSize(BSize(icon->Bounds().Width(), icon->Bounds().Height()));
    SetExplicitSize(BSize(15, 15)); // Standard Deskbar icon size
}

void DeskbarView::DetachedFromWindow() {
    printf("DeskbarView::DetachedFromWindow\n");
    BView::DetachedFromWindow();
}

void DeskbarView::Draw(BRect updateRect) {
    BView::Draw(updateRect);
    // Fill with parent's color or transparent
    if (Parent()) {
        SetLowColor(Parent()->ViewColor());
        FillRect(Bounds(), B_SOLID_LOW);
    } else {
        SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
        FillRect(Bounds(), B_SOLID_LOW);
    }

    SetHighColor(0, 0, 0); // Black
    StrokeRect(Bounds()); // Draw a border for visibility

    // Draw a placeholder string or an icon
    const char* text = "Py";
    font_height fh;
    GetFontHeight(&fh);
    float textWidth = StringWidth(text);
    float x = (Bounds().Width() - textWidth) / 2;
    // Center vertically, considering Haiku's font rendering
    float y = (Bounds().Height() - (fh.ascent + fh.descent)) / 2 + fh.ascent + 1;
    DrawString(text, BPoint(x, y));

    // if (icon) DrawBitmap(icon, BPoint(0,0));
    // Set ToolTip
    // SetToolTip("Default Tooltip"); // Example
}

void DeskbarView::MessageReceived(BMessage* message) {
    switch (message->what) {
        case 'VICN': // View Icon Change
        {
            const char* path = NULL;
            if (message->FindString("image_path", &path) == B_OK) {
                printf("DeskbarView::MessageReceived: VICN - Path: %s\n", path);
                // TODO: Load BBitmap from path and update the view's icon
                // delete fIcon; // fIcon would be a BBitmap* member
                // fIcon = BTranslationUtils::GetBitmapFile(path);
                // if (fIcon && fIcon->IsValid()) {
                //    Invalidate(); // Trigger redraw
                //    printf("Icon updated from %s\n", path);
                // } else {
                //    printf("Failed to load icon from %s\n", path);
                //    delete fIcon;
                //    fIcon = NULL;
                // }
                printf("DeskbarView: Actual icon loading from path is needed.\n");
            }
            break;
        }
        case 'VTIL': // View Title Change
        {
            const char* new_title = NULL;
            if (message->FindString("title", &new_title) == B_OK) {
                printf("DeskbarView::MessageReceived: VTIL - Title: %s\n", new_title);
                // SetToolTip(new_title); // BView's method for tooltips
                // Invalidate(); // May not be necessary for tooltip
                printf("DeskbarView: Tooltip set to: %s\n", new_title);
            }
            break;
        }
        default:
            BView::MessageReceived(message);
            break;
    }
}


void DeskbarView::MouseDown(BPoint point) {
    printf("DeskbarView::MouseDown at (%.1f, %.1f)\n", point.x, point.y);
    BMessage* currentMsg = Window()->CurrentMessage(); // The mouse down message
    uint32 buttons = 0;
    if (currentMsg->FindInt32("buttons", (int32*)&buttons) != B_OK) {
        printf("Could not get buttons for MouseDown event\n");
        return;
    }

    if (buttons & B_PRIMARY_MOUSE_BUTTON) {
        // Handle left click - e.g., send a message to the app to show main window or menu
        // BMessage openMsg('LCLK'); // Define 'LCLK' (Left Click) in your app
        // be_app_messenger.SendMessage(&openMsg); // Send to App
        (new BAlert("Pystray", "Pystray Helper Left Clicked!", "OK"))->Go(NULL);
    } else if (buttons & B_SECONDARY_MOUSE_BUTTON) {
        // Handle right click - e.g., show a context menu
        BPopUpMenu* menu = new BPopUpMenu("DeskbarViewMenu", false, false);
        // TODO: Menu items should be dynamically populated based on pystray_update_menu()
        menu->AddItem(new BMenuItem("About Pystray Helper...", new BMessage('ABUT'))); // Handled by MainWindow
        menu->AddSeparatorItem();
        menu->AddItem(new BMenuItem("Item 1 (Placeholder)", new BMessage('ITM1')));   // Could be sent to App
        menu->AddItem(new BMenuItem("Item 2 (Placeholder)", new BMessage('ITM2')));
        menu->AddSeparatorItem();
        menu->AddItem(new BMenuItem("Quit Helper", new BMessage(B_QUIT_REQUESTED))); // Handled by App

        // Target App for most actions, MainWindow for 'ABUT' if it's specific there
        menu->SetTargetForItems(be_app_messenger);

        ConvertToScreen(&point);
        menu->Go(point, true, true, true); // Async display
    }
}

// This global C function is the entry point for the Deskbar
// to instantiate the replicant. It must return a BView object.
extern "C" _EXPORT BView* instantiate_deskbar_item(void) {
    // Standard Deskbar icon size is typically 16x16 pixels.
    // The BRect for the view should be (0,0) to (width-1, height-1).
    BRect frame(0, 0, 15, 15); // So, 16x16 pixels.
    printf("instantiate_deskbar_item() called in PystrayHaikuHelper\n");
    return new DeskbarView(frame);
}

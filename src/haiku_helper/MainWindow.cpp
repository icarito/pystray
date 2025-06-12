#include "MainWindow.h"
#include "App.h"         // For APP_SIGNATURE
#include "DeskbarView.h" // For DESKBAR_VIEW_NAME

#include <Application.h> // For be_app, be_app_messenger
#include <Deskbar.h>
#include <Roster.h>      // For app_info, running_app_info
#include <Entry.h>
#include <Path.h>
#include <Alert.h>
#include <Notification.h> // Added for BNotification
#include <stdio.h>        // For printf
#include <string.h>       // For strerror, isprint

// Helper function from App.cpp (ideally in a shared utility header)
// For now, duplicate or ensure it's accessible if this were split differently.
// Helper to convert message 'what' to a string for logging
static void msg_what_to_string_mw(uint32 what, char* buffer, size_t buffer_size) {
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


MainWindow::MainWindow()
    : BWindow(BRect(100, 100, 350, 250), "Pystray Helper Window", B_TITLED_WINDOW,
              B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
      fDeskbarView(NULL),
      fReplicantAdded(false) {
    printf("MainWindow: Constructor called and initialized.\n");

    _AddReplicantToDeskbar();

    if (fReplicantAdded) {
        printf("MainWindow: Constructor - Replicant was successfully added. Hiding main window.\n");
        if (!IsHidden()) Hide();
    } else {
        printf("MainWindow: Constructor - Replicant was NOT added. Hiding main window by policy.\n");
        if (!IsHidden()) Hide();
    }
    printf("MainWindow: Constructor finished.\n");
}

MainWindow::~MainWindow() {
    printf("MainWindow: Destructor called.\n");
}

void MainWindow::_AddReplicantToDeskbar() {
    printf("MainWindow: _AddReplicantToDeskbar - Starting.\n");
    BDeskbar deskbar;
    if (!deskbar.IsRunning()) {
        fprintf(stderr, "MainWindow: _AddReplicantToDeskbar - ERROR: Deskbar is not running.\n");
        return;
    }

    printf("MainWindow: _AddReplicantToDeskbar - Attempting to add replicant by app_ref (view name for check: '%s').\n", DESKBAR_VIEW_NAME);

    if (deskbar.HasItem(DESKBAR_VIEW_NAME)) {
        printf("MainWindow: _AddReplicantToDeskbar - Replicant '%s' already in Deskbar.\n", DESKBAR_VIEW_NAME);
        fReplicantAdded = true;
        return;
    }

    app_info appInfo;
    status_t status = be_app->GetAppInfo(&appInfo);
    if (status != B_OK) {
        fprintf(stderr, "MainWindow: _AddReplicantToDeskbar - ERROR getting app info: %s\n", strerror(status));
        return;
    }
    entry_ref app_ref = appInfo.ref;
    // Note: Printing app_ref.name might be misleading if it's just the app executable name.
    // The replicant is identified by DESKBAR_VIEW_NAME or its archived signature.
    printf("MainWindow: _AddReplicantToDeskbar - Using app_ref for app signature: %s.\n", appInfo.signature);

    status = deskbar.AddItem(&app_ref, NULL);
    printf("MainWindow: _AddReplicantToDeskbar - AddItem returned: %s (0x%lx)\n", strerror(status), status);

    if (status == B_OK) {
        printf("MainWindow: _AddReplicantToDeskbar - Replicant '%s' (using app_ref) added to Deskbar successfully.\n", DESKBAR_VIEW_NAME);
        fReplicantAdded = true;
    } else {
        fprintf(stderr, "MainWindow: _AddReplicantToDeskbar - ERROR adding replicant '%s' to Deskbar: %s\n", DESKBAR_VIEW_NAME, strerror(status));
        fReplicantAdded = false;
    }
}

void MainWindow::_RemoveReplicantFromDeskbar() {
    printf("MainWindow: _RemoveReplicantFromDeskbar - Starting for item '%s'.\n", DESKBAR_VIEW_NAME);
    if (!fReplicantAdded && !BDeskbar().HasItem(DESKBAR_VIEW_NAME)) { // Check current Deskbar state too
        printf("MainWindow: _RemoveReplicantFromDeskbar - Replicant not marked as added (fReplicantAdded=false) AND not found in Deskbar. No action taken.\n");
        return;
    }

    BDeskbar deskbar;
    if (!deskbar.IsRunning()) {
        fprintf(stderr, "MainWindow: _RemoveReplicantFromDeskbar - ERROR: Deskbar is not running.\n");
        return;
    }

    printf("MainWindow: _RemoveReplicantFromDeskbar - Attempting to remove item '%s'.\n", DESKBAR_VIEW_NAME);
    status_t err = deskbar.RemoveItem(DESKBAR_VIEW_NAME);
    printf("MainWindow: _RemoveReplicantFromDeskbar - RemoveItem returned: %s (0x%lx)\n", strerror(err), err);

    if (err == B_OK) {
        printf("MainWindow: _RemoveReplicantFromDeskbar - Replicant '%s' removed successfully.\n", DESKBAR_VIEW_NAME);
        fReplicantAdded = false;
    } else {
        fprintf(stderr, "MainWindow: _RemoveReplicantFromDeskbar - ERROR removing item '%s': %s\n", DESKBAR_VIEW_NAME, strerror(err));
        if (err == B_ENTRY_NOT_FOUND) { // If Deskbar says it's not there, update our state.
            printf("MainWindow: _RemoveReplicantFromDeskbar - Item was not found, perhaps already removed by user.\n");
            fReplicantAdded = false;
        }
    }
}

bool MainWindow::QuitRequested() {
    printf("MainWindow: QuitRequested() called. Posting B_QUIT_REQUESTED to application.\n");
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}

void MainWindow::MessageReceived(BMessage* message) {
    char whatStr[12]; // Buffer for 'what' code as string
    msg_what_to_string_mw(message->what, whatStr, sizeof(whatStr));
    printf("MainWindow: MessageReceived - what: %s\n", whatStr);
    // message->PrintToStream(); // Uncomment for very detailed BMessage debugging

    switch (message->what) {
        case 'INIT':
            printf("MainWindow: Message 'INIT' received. Action: Calling _AddReplicantToDeskbar to ensure it's present.\n");
            _AddReplicantToDeskbar();
            if (fReplicantAdded && IsHidden() == false && LockLooper()) {
                printf("MainWindow: 'INIT' - Replicant present and window is visible, so hiding window.\n");
                Hide();
                UnlockLooper();
            } else if (fReplicantAdded && IsHidden()) {
                 printf("MainWindow: 'INIT' - Replicant present and window already hidden.\n");
            } else if (!fReplicantAdded) {
                 printf("MainWindow: 'INIT' - Replicant still not added after explicit call.\n");
            }
            break;
        case 'SHOW': // Corresponds to MSG_SHOW_DESKBAR_ITEM / pystray_show_icon
            printf("MainWindow: Message 'SHOW' (Show Deskbar Item) received. Action: Calling _AddReplicantToDeskbar.\n");
            _AddReplicantToDeskbar();
            break;
        case 'HIDE': // Corresponds to MSG_HIDE_DESKBAR_ITEM / pystray_hide_icon
            printf("MainWindow: Message 'HIDE' (Hide Deskbar Item) received. Action: Calling _RemoveReplicantFromDeskbar.\n");
            _RemoveReplicantFromDeskbar();
            break;
        case 'ICON': // Corresponds to MSG_UPDATE_ICON / pystray_update_icon
        {
            const char* path = NULL;
            if (message->FindString("image_path", &path) == B_OK && path) {
                printf("MainWindow: Message 'ICON' (Update Icon) received. Path: '%s'. Action: Attempting to forward 'VICN' to DeskbarView(s).\n", path);

                BDeskbar deskbar;
                if (!deskbar.IsRunning()) {
                    fprintf(stderr, "MainWindow: 'ICON' handler - Deskbar not running, cannot forward message.\n");
                    break;
                }
                BMessage updateIconMsg('VICN'); // Message type for DeskbarView
                updateIconMsg.AddString("icon_path", path);

                extern const char* APP_SIGNATURE; // Defined in App.cpp
                int32 count = deskbar.CountItems(APP_SIGNATURE);
                printf("MainWindow: 'ICON' handler - Found %d item(s) with signature '%s' to potentially update.\n", (int)count, APP_SIGNATURE);
                if (count == 0) {
                     printf("MainWindow: 'ICON' handler - No replicants found with our signature. Cannot send 'VICN'.\n");
                }
                for (int32 i = 0; i < count; ++i) {
                     BMessenger targetMessenger;
                     if (deskbar.GetMessenger(APP_SIGNATURE, i, &targetMessenger) == B_OK) {
                         status_t send_err = targetMessenger.SendMessage(&updateIconMsg);
                         printf("MainWindow: 'ICON' handler - Sent 'VICN' to replicant instance %ld. Path: '%s'. Status: %s\n", i, path, strerror(send_err));
                     } else {
                         fprintf(stderr, "MainWindow: 'ICON' handler - ERROR: Failed to get BMessenger for replicant instance %ld.\n", i);
                     }
                 }
            } else {
                fprintf(stderr, "MainWindow: Message 'ICON' - ERROR: Could not find 'image_path' string or path is NULL.\n");
            }
            break;
        }
        case 'TITL': // Corresponds to MSG_UPDATE_TITLE / pystray_update_title
        {
            const char* new_title = NULL;
            if (message->FindString("title", &new_title) == B_OK) {
                printf("MainWindow: Message 'TITL' (Update Title) received. Title: '%s'. Action: Attempting to forward 'VTIL' to DeskbarView(s).\n", new_title ? new_title : "NULL_TITLE");

                BDeskbar deskbar;
                 if (!deskbar.IsRunning()) {
                    fprintf(stderr, "MainWindow: 'TITL' handler - Deskbar not running, cannot forward message.\n");
                    break;
                }
                BMessage updateTitleMsg('VTIL'); // Message type for DeskbarView
                updateTitleMsg.AddString("title", new_title ? new_title : "");

                extern const char* APP_SIGNATURE;
                int32 count = deskbar.CountItems(APP_SIGNATURE);
                printf("MainWindow: 'TITL' handler - Found %d item(s) with signature '%s' to potentially update title.\n", (int)count, APP_SIGNATURE);
                 if (count == 0) {
                     printf("MainWindow: 'TITL' handler - No replicants found with our signature. Cannot send 'VTIL'.\n");
                }
                for (int32 i = 0; i < count; ++i) {
                     BMessenger targetMessenger;
                     if (deskbar.GetMessenger(APP_SIGNATURE, i, &targetMessenger) == B_OK) {
                         status_t send_err = targetMessenger.SendMessage(&updateTitleMsg);
                         printf("MainWindow: 'TITL' handler - Sent 'VTIL' to replicant instance %ld. Title: '%s'. Status: %s\n", i, new_title ? new_title : "", strerror(send_err));
                     } else {
                         fprintf(stderr, "MainWindow: 'TITL' handler - ERROR: Failed to get BMessenger for replicant instance %ld.\n", i);
                     }
                 }
            } else {
                fprintf(stderr, "MainWindow: Message 'TITL' - ERROR: Could not find 'title' string.\n");
            }
            break;
        }
        case 'NOTI': // Corresponds to MSG_SHOW_NOTIFICATION / pystray_notify
        {
            const char* msg_text = NULL;
            const char* notif_title = NULL;
            if (message->FindString("message", &msg_text) == B_OK &&
                message->FindString("notification_title", &notif_title) == B_OK) {
                printf("MainWindow: Message 'NOTI' (Show Notification) received. Title: '%s', Message: '%s'. Action: Displaying BNotification.\n",
                       notif_title ? notif_title : "DefaultTitle", msg_text ? msg_text : "DefaultMessage");

                BNotification notification(B_INFORMATION_NOTIFICATION);
                notification.SetGroup("Pystray");
                notification.SetTitle(notif_title ? notif_title : "Pystray Notification");
                notification.SetContent(msg_text ? msg_text : "");

                status_t send_status = notification.Send();
                if (send_status != B_OK) {
                    fprintf(stderr, "MainWindow: 'NOTI' - ERROR sending BNotification: %s\n", strerror(send_status));
                } else {
                    printf("MainWindow: 'NOTI' - BNotification sent successfully.\n");
                }
            } else {
                fprintf(stderr, "MainWindow: Message 'NOTI' - ERROR: Missing 'message' or 'notification_title' strings.\n");
            }
            break;
        }
        case 'ABUT':
            printf("MainWindow: Message 'ABUT' received. Action: Showing 'About' alert.\n");
            (new BAlert("About", "Pystray Haiku Helper\n\n"
                                 "Provides Deskbar integration for pystray.", "OK"))->Go(NULL);
            break;
        default:
            printf("MainWindow: Message unhandled by MainWindow (what: %s), passing to BWindow::MessageReceived.\n", whatStr);
            BWindow::MessageReceived(message);
            break;
    }
    printf("MainWindow: MessageReceived - Finished processing what: %s.\n", whatStr);
}

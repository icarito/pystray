#include "MainWindow.h"
#include "App.h"         // For APP_SIGNATURE, App class cast
#include "DeskbarView.h" // For DESKBAR_VIEW_NAME, and fDeskbarView member

#include <Application.h> // For be_app, be_app_messenger
#include <Deskbar.h>
#include <Roster.h>      // For app_info
#include <Entry.h>
#include <Path.h>        // Not strictly needed with current code, but often useful
#include <Alert.h>
#include <Notification.h>
#include <stdio.h>       // For printf
#include <string.h>      // For strerror
#include <ctype.h>       // For isprint (used in msg_what_to_string_mw)

// Message 'what' constants for communication with DeskbarView
// These should match what DeskbarView::MessageReceived expects
const uint32 VIEW_UPDATE_ICON_MSG = 'VICN';
const uint32 VIEW_UPDATE_TITLE_MSG = 'VTIL';


// Helper to convert message 'what' to a string for logging
static void msg_what_to_string_mw(uint32 what, char* buffer, size_t buffer_size) {
    char c1 = (what >> 24) & 0xFF;
    char c2 = (what >> 16) & 0xFF;
    char c3 = (what >> 8) & 0xFF;
    char c4 = what & 0xFF;

    if (isprint(c1) && isprint(c2) && isprint(c3) && isprint(c4)) {
        snprintf(buffer, buffer_size, "'%c%c%c%c'", c1, c2, c3, c4);
    } else {
        snprintf(buffer, buffer_size, "0x%08x", (unsigned int)what);
    }
}

MainWindow::MainWindow()
    : BWindow(BRect(100, 100, 350, 250), "Pystray Helper Window", B_TITLED_WINDOW,
              B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
      fDeskbarView(NULL),
      fReplicantAdded(false),
      fDeskbarReplicantID(-1) { // Initialize ID to an invalid state
    printf("MainWindow: Constructor called.\n");

    // Create the DeskbarView instance that this window will manage
    // This view is then added to the Deskbar by _AddReplicantToDeskbar
    if (!fDeskbarView) {
        printf("MainWindow: Constructor - fDeskbarView is NULL, creating new DeskbarView instance.\n");
        fDeskbarView = new DeskbarView(BRect(0, 0, 15, 15)); // Standard 16x16 icon size
    }

    _AddReplicantToDeskbar();

    if (fReplicantAdded) {
        printf("MainWindow: Constructor - Replicant was successfully added. Hiding main window.\n");
        if (!IsHidden()) Hide();
    } else {
        printf("MainWindow: Constructor - Replicant was NOT added. Hiding main window by policy.\n");
        // Even if adding fails, this window is not meant to be user-facing.
        if (!IsHidden()) Hide();
    }
    printf("MainWindow: Constructor finished.\n");
}

MainWindow::~MainWindow() {
    printf("MainWindow: Destructor called.\n");
    // If the view was added by instance, it should be removed.
    // _RemoveReplicantFromDeskbar(); // This might be too late or cause issues if app is quitting.
    // BDeskbar should handle removing it when the app quits if added by app signature.
    // If added as a BView instance, Deskbar might not remove it automatically when app quits.
    // For safety, let's try to remove if it was added and we have an ID.
    if (fReplicantAdded && fDeskbarReplicantID != -1) {
         printf("MainWindow: Destructor - Attempting to remove replicant from Deskbar.\n");
        _RemoveReplicantFromDeskbar();
    }
    delete fDeskbarView; // MainWindow owns fDeskbarView instance if created this way
    fDeskbarView = NULL;
}

void MainWindow::_AddReplicantToDeskbar() {
    printf("MainWindow: _AddReplicantToDeskbar - Starting.\n");
    BDeskbar deskbar;
    if (!deskbar.IsRunning()) {
        fprintf(stderr, "MainWindow: _AddReplicantToDeskbar - ERROR: Deskbar is not running.\n");
        return;
    }

    if (!fDeskbarView) {
        fprintf(stderr, "MainWindow: _AddReplicantToDeskbar - ERROR: fDeskbarView is NULL. Cannot add.\n");
        return;
    }

    // Using the name of the fDeskbarView instance for checking if it exists.
    // Note: This might not be perfectly reliable if multiple instances could share a name,
    // but for a single app instance, it's usually okay.
    printf("MainWindow: _AddReplicantToDeskbar - Attempting to add view '%s'.\n", fDeskbarView->Name());

    if (deskbar.HasItem(fDeskbarView->Name())) {
        printf("MainWindow: _AddReplicantToDeskbar - An item named '%s' already in Deskbar.\n", fDeskbarView->Name());
        // Try to get its ID, or remove and re-add if uncertain it's our current fDeskbarView instance.
        // For simplicity now, assume if name matches and fReplicantAdded is true, it's ours.
        if (fReplicantAdded) {
             printf("MainWindow: _AddReplicantToDeskbar - Already marked as added. Assuming it's the correct instance.\n");
            return;
        }
        // If not marked as added by us, but exists, it could be a leftover.
        printf("MainWindow: _AddReplicantToDeskbar - Item exists but fReplicantAdded is false. Removing existing then re-adding.\n");
        deskbar.RemoveItem(fDeskbarView->Name()); // Remove by name
    }

    fDeskbarReplicantID = -1; // Reset ID
    status_t status = deskbar.AddItem(fDeskbarView, &fDeskbarReplicantID);
    // After this call, fDeskbarView is now owned by Deskbar in terms of window hierarchy.
    // MainWindow should NOT delete fDeskbarView if AddItem is successful, UNLESS RemoveItem is called.
    // However, if MainWindow is the one creating fDeskbarView, it's responsible for it if AddItem fails,
    // or after it's removed from Deskbar and before MainWindow itself is deleted.
    // This ownership model is tricky. A common pattern is Deskbar loads from an add-on (app_ref).
    // If adding a BView* directly, the BView's lifetime needs careful management.
    // For now, assume MainWindow deletes fDeskbarView in its destructor, implying it should be removed first.

    printf("MainWindow: _AddReplicantToDeskbar - AddItem returned: %s (status: %d, 0x%x), ID: %d\n",
           strerror(status), (int)status, (unsigned int)status, (int)fDeskbarReplicantID);

    if (status == B_OK) {
        printf("MainWindow: _AddReplicantToDeskbar - fDeskbarView added successfully with ID: %d\n", (int)fDeskbarReplicantID);
        fReplicantAdded = true;
        App* app = static_cast<App*>(be_app);
        if (app) {
            app->SetReplicantMessenger(BMessenger(fDeskbarView));
            if (app->GetReplicantMessenger().IsValid()) {
                printf("MainWindow: _AddReplicantToDeskbar - Replicant messenger set in App and is valid.\n");
            } else {
                // This can happen if fDeskbarView is not yet in a window or has no looper.
                // Deskbar should add it to its window, so BMessenger(fDeskbarView) should become valid.
                // May need a small delay or to get messenger after window attachment if issues persist.
                fprintf(stderr, "MainWindow: _AddReplicantToDeskbar - WARN: Replicant messenger set in App but BMessenger(fDeskbarView) is invalid immediately after AddItem.\n");
            }
        } else {
            fprintf(stderr, "MainWindow: _AddReplicantToDeskbar - ERROR: be_app is not an App instance or is NULL.\n");
        }
    } else {
        fprintf(stderr, "MainWindow: _AddReplicantToDeskbar - ERROR adding fDeskbarView to Deskbar: %s\n", strerror(status));
        fReplicantAdded = false;
        fDeskbarReplicantID = -1;
        // If AddItem fails, MainWindow still owns fDeskbarView. It will be deleted in ~MainWindow.
    }
}

void MainWindow::_RemoveReplicantFromDeskbar() {
    printf("MainWindow: _RemoveReplicantFromDeskbar - Starting for ID %d (View Name: '%s').\n",
           (int)fDeskbarReplicantID, fDeskbarView ? fDeskbarView->Name() : "N/A");

    if (!fReplicantAdded && fDeskbarReplicantID == -1) {
        printf("MainWindow: _RemoveReplicantFromDeskbar - Replicant not marked as added and ID is invalid. No action taken.\n");
        return;
    }

    BDeskbar deskbar;
    if (!deskbar.IsRunning()) {
        fprintf(stderr, "MainWindow: _RemoveReplicantFromDeskbar - ERROR: Deskbar is not running.\n");
        return;
    }

    status_t err = B_ERROR;
    if (fDeskbarReplicantID != -1) {
        printf("MainWindow: _RemoveReplicantFromDeskbar - Attempting to remove item by ID %d.\n", (int)fDeskbarReplicantID);
        err = deskbar.RemoveItem(fDeskbarReplicantID);
    } else if (fDeskbarView && deskbar.HasItem(fDeskbarView->Name())) {
        printf("MainWindow: _RemoveReplicantFromDeskbar - ID unknown, attempting to remove item by name '%s'.\n", fDeskbarView->Name());
        err = deskbar.RemoveItem(fDeskbarView->Name());
    } else {
        printf("MainWindow: _RemoveReplicantFromDeskbar - No valid ID and no fDeskbarView or item not found by name. Cannot remove.\n");
        fReplicantAdded = false;
        return;
    }

    printf("MainWindow: _RemoveReplicantFromDeskbar - RemoveItem returned: %s (status: %d, 0x%x)\n",
           strerror(err), (int)err, (unsigned int)err);

    if (err == B_OK) {
        printf("MainWindow: _RemoveReplicantFromDeskbar - Replicant (ID: %d) removed successfully.\n", (int)fDeskbarReplicantID);
        fReplicantAdded = false;
        fDeskbarReplicantID = -1;
        App* app = static_cast<App*>(be_app);
        if (app) {
            app->SetReplicantMessenger(BMessenger());
            printf("MainWindow: _RemoveReplicantFromDeskbar - Cleared replicant messenger in App.\n");
        }
        // After successful removal, fDeskbarView is no longer in Deskbar's hierarchy.
        // MainWindow is now solely responsible for deleting it if it created it.
        // This happens in ~MainWindow.
    } else {
        fprintf(stderr, "MainWindow: _RemoveReplicantFromDeskbar - ERROR removing replicant (ID: %d): %s\n", (int)fDeskbarReplicantID, strerror(err));
        if (err == B_ENTRY_NOT_FOUND) {
            printf("MainWindow: _RemoveReplicantFromDeskbar - Item was not found by ID/name, perhaps already removed.\n");
            fReplicantAdded = false;
            fDeskbarReplicantID = -1;
        }
    }
}

bool MainWindow::QuitRequested() {
    printf("MainWindow: QuitRequested() called. Posting B_QUIT_REQUESTED to application.\n");
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}

void MainWindow::MessageReceived(BMessage* message) {
    char whatStr[12];
    msg_what_to_string_mw(message->what, whatStr, sizeof(whatStr));
    printf("MainWindow: MessageReceived - what: %s\n", whatStr);

    switch (message->what) {
        case 'INIT':
            printf("MainWindow: Message 'INIT' received. Action: Calling _AddReplicantToDeskbar.\n");
            _AddReplicantToDeskbar(); // Ensure it's added
            if (fReplicantAdded && IsHidden() == false && LockLooper()) {
                printf("MainWindow: 'INIT' - Replicant present and window visible, hiding.\n");
                Hide();
                UnlockLooper();
            }
            break;
        case 'SHOW':
            printf("MainWindow: Message 'SHOW' (Show Deskbar Item) received. Action: Calling _AddReplicantToDeskbar.\n");
            _AddReplicantToDeskbar();
            break;
        case 'HIDE':
            printf("MainWindow: Message 'HIDE' (Hide Deskbar Item) received. Action: Calling _RemoveReplicantFromDeskbar.\n");
            _RemoveReplicantFromDeskbar();
            break;
        case 'ICON': // This is PYSTRAY_UPDATE_ICON_MSG
        {
            printf("MainWindow: Message 'ICON' (Update Icon) received.\n");
            const char* path = NULL;
            if (message->FindString("icon_path", &path) == B_OK && path) {
                App* app = static_cast<App*>(be_app);
                if (!app) {
                     fprintf(stderr, "MainWindow: 'ICON' - ERROR: be_app is not an App instance or NULL.\n");
                     break;
                }
                BMessenger targetMessenger = app->GetReplicantMessenger();
                if (targetMessenger.IsValid()) {
                    printf("MainWindow: 'ICON' - Path: '%s'. Forwarding 'VICN' to App's replicant messenger.\n", path);
                    BMessage updateViewMsg(VIEW_UPDATE_ICON_MSG);
                    updateViewMsg.AddString("icon_path", path);
                    status_t send_err = targetMessenger.SendMessage(&updateViewMsg);
                    if (send_err != B_OK) {
                        fprintf(stderr, "MainWindow: 'ICON' - Failed to send 'VICN' to DeskbarView: %s\n", strerror(send_err));
                    } else {
                        printf("MainWindow: 'ICON' - Sent 'VICN' to DeskbarView with path: %s\n", path);
                    }
                } else {
                    fprintf(stderr, "MainWindow: 'ICON' - ERROR: Invalid replicant messenger in App, cannot send 'VICN'.\n");
                }
            } else {
                fprintf(stderr, "MainWindow: Message 'ICON' - ERROR: Could not find 'icon_path' string or path is NULL.\n");
            }
            break;
        }
        case 'TITL': // This is PYSTRAY_UPDATE_TITLE_MSG
        {
            printf("MainWindow: Message 'TITL' (Update Title) received.\n");
            const char* new_title = NULL;
            // FindString can succeed and set new_title to NULL if the attribute exists but is empty/corrupt.
            if (message->FindString("title", &new_title) == B_OK) {
                App* app = static_cast<App*>(be_app);
                 if (!app) {
                     fprintf(stderr, "MainWindow: 'TITL' - ERROR: be_app is not an App instance or NULL.\n");
                     break;
                }
                BMessenger targetMessenger = app->GetReplicantMessenger();
                if (targetMessenger.IsValid()) {
                    printf("MainWindow: 'TITL' - Title: '%s'. Forwarding 'VTIL' to App's replicant messenger.\n", new_title ? new_title : "NULL_TITLE_RECEIVED");
                    BMessage updateViewMsg(VIEW_UPDATE_TITLE_MSG);
                    updateViewMsg.AddString("title", new_title ? new_title : ""); // Send empty for NULL
                    status_t send_err = targetMessenger.SendMessage(&updateViewMsg);
                    if (send_err != B_OK) {
                        fprintf(stderr, "MainWindow: 'TITL' - Failed to send 'VTIL' to DeskbarView: %s\n", strerror(send_err));
                    } else {
                        printf("MainWindow: 'TITL' - Sent 'VTIL' to DeskbarView with title: %s\n", new_title ? new_title : "NULL_TITLE_SENT");
                    }
                } else {
                    fprintf(stderr, "MainWindow: 'TITL' - ERROR: Invalid replicant messenger in App, cannot send 'VTIL'.\n");
                }
            } else {
                fprintf(stderr, "MainWindow: Message 'TITL' - ERROR: Could not find 'title' string.\n");
            }
            break;
        }
        case 'NOTI': // This is MSG_SHOW_NOTIFICATION
        {
            printf("MainWindow: Message 'NOTI' (Show Notification) received.\n");
            const char* msg_text = NULL;
            const char* notif_title = NULL;
            if (message->FindString("message", &msg_text) == B_OK &&
                message->FindString("notification_title", &notif_title) == B_OK) {
                printf("MainWindow: 'NOTI' - Title: '%s', Message: '%s'. Displaying BNotification.\n",
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

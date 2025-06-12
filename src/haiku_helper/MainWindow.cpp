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
#include <stdio.h>       // For printf

MainWindow::MainWindow()
    : BWindow(BRect(100, 100, 350, 250), "Pystray Helper Window", B_TITLED_WINDOW,
              B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
      fDeskbarView(NULL),
      fReplicantAdded(false) {
    printf("MainWindow constructor\n");
    // The window is primarily for hosting the replicant logic initially
    // and potentially for debugging or complex interactions later.
    // For a simple Deskbar replicant, this window might be hidden.

    _AddReplicantToDeskbar();

    // If the replicant is successfully added, this window can be hidden.
    // If it's not added, we might want to show this window with an error or instructions.
    if (fReplicantAdded) {
        Hide();
    } else {
        // (new BAlert("Pystray Helper", "Could not add icon to Deskbar.", "OK"))->Go(NULL);
        // For now, we'll still hide it, as the app's main presence is the Deskbar item.
    // Hide(); // Window is hidden by default after _AddReplicantToDeskbar or if it fails
    if (IsHidden()) {
        printf("MainWindow constructor: Window is hidden.\n");
    } else {
        printf("MainWindow constructor: Window is visible. This is unusual for a replicant host.\n");
        // Ensure it's hidden if replicant was added, otherwise it might be shown for error.
        if (fReplicantAdded) Hide(); else Show(); // Show if error and no replicant
    }
    }
    // If you want to test the DeskbarView directly in this window (not as a replicant):
    // BRect viewRect = Bounds();
    // fDeskbarView = new DeskbarView(viewRect);
    // AddChild(fDeskbarView);
    // Show();
}

MainWindow::~MainWindow() {
    printf("MainWindow destructor\n");
    // _RemoveReplicantFromDeskbar(); // Clean up Deskbar if app is quitting this way
    // Note: BApplication usually handles deleting BWindows in its list.
    // If fDeskbarView was added as a child, it's deleted by BWindow.
}

void MainWindow::_AddReplicantToDeskbar() {
    BDeskbar deskbar;
    if (!deskbar.IsRunning()) {
        printf("Deskbar is not running.\n");
        // (new BAlert("Error", "Deskbar is not running. Cannot add replicant.", "OK"))->Go(NULL);
        return;
    }

    // Check if the replicant is already there by its view name
    if (deskbar.HasItem(DESKBAR_VIEW_NAME)) {
        printf("Replicant '%s' already in Deskbar.\n", DESKBAR_VIEW_NAME);
        fReplicantAdded = true; // Assume it's ours or functionally equivalent
        return;
    }

    // Get the application's entry_ref to add the replicant
    app_info appInfo;
    status_t status = be_app->GetAppInfo(&appInfo);
    if (status != B_OK) {
        printf("Error getting app info: %s\n", strerror(status));
        // (new BAlert("Error", "Could not get application info to add replicant.", "OK"))->Go(NULL);
        return;
    }
    entry_ref app_ref = appInfo.ref;

    // Add the replicant by signature. Deskbar will look for instantiate_deskbar_item.
    // The 'name' parameter to AddItem can be NULL if the replicant is self-named (via Archive)
    // or if we pass the app_ref.
    status = deskbar.AddItem(&app_ref, NULL); // Pass app_ref, Deskbar finds exported func
                                          // Or use: deskbar.AddItem(APP_SIGNATURE); if app is registered
    if (status != B_OK) {
        printf("Error adding replicant to Deskbar: %s\n", strerror(status));
        // (new BAlert("Error", "Could not add replicant to Deskbar.", "OK"))->Go(NULL);
    } else {
        printf("Replicant '%s' added to Deskbar successfully.\n", DESKBAR_VIEW_NAME);
        fReplicantAdded = true;
    }
}

void MainWindow::_RemoveReplicantFromDeskbar() {
    // This is more for explicit removal if the app has a "remove from deskbar" option
    // or during uninstallation. Typically, users remove replicants themselves.
    if (!fReplicantAdded) return;

    BDeskbar deskbar;
    if (deskbar.IsRunning() && deskbar.HasItem(DESKBAR_VIEW_NAME)) {
        status_t err = deskbar.RemoveItem(DESKBAR_VIEW_NAME);
        if (err == B_OK) {
            printf("Replicant '%s' removed from Deskbar.\n", DESKBAR_VIEW_NAME);
            fReplicantAdded = false;
        } else {
            printf("Error removing replicant '%s' from Deskbar: %s\n", DESKBAR_VIEW_NAME, strerror(err));
        }
    }
}

bool MainWindow::QuitRequested() {
    printf("MainWindow::QuitRequested()\n");
    // This window is usually hidden. Quitting is typically initiated
    // from the DeskbarView's context menu or by a message from pystray.
    // _RemoveReplicantFromDeskbar(); // Optional: remove replicant when main window quits
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true; // BApplication will handle actual quitting
}

void MainWindow::MessageReceived(BMessage* message) {
    switch (message->what) {
        case 'INIT': // Sent from pystray_init via App
            printf("MainWindow::MessageReceived: INIT\n");
            // Ensure replicant is added. App::ReadyToRun usually handles initial MainWindow.
            // This message ensures it happens if pystray_init is called when app is already running.
            _AddReplicantToDeskbar();
            if (fReplicantAdded && IsHidden() == false && Lock()) {
                Hide();
                Unlock();
            }
            break;
        case 'SHOW':
            printf("MainWindow::MessageReceived: SHOW icon\n");
            // This implies ensuring the replicant is in the Deskbar.
            _AddReplicantToDeskbar();
            // If the view itself can be hidden/shown independently of being in Deskbar:
            // if (fDeskbarView && fDeskbarView->LockLooper()) {
            //     if (fDeskbarView->IsHidden()) fDeskbarView->Show();
            //     fDeskbarView->UnlockLooper();
            // }
            break;
        case 'HIDE':
            printf("MainWindow::MessageReceived: HIDE icon\n");
            // This implies removing the replicant from the Deskbar.
            _RemoveReplicantFromDeskbar();
            // If the view itself can be hidden/shown:
            // if (fDeskbarView && fDeskbarView->LockLooper()) {
            //     if (!fDeskbarView->IsHidden()) fDeskbarView->Hide();
            //     fDeskbarView->UnlockLooper();
            // }
            break;
        case 'ICON':
        {
            const char* path = NULL;
            if (message->FindString("image_path", &path) == B_OK) {
                printf("MainWindow::MessageReceived: ICON update - Path: %s\n", path);
                // TODO: Relay this to DeskbarView to load and display the new icon
                // This might involve creating a BMessage and sending it to fDeskbarView
                // or calling a method on fDeskbarView if we have a pointer and it's safe.
                // For now, DeskbarView is not directly managed by MainWindow after attachment.
                // This requires a more robust way to communicate with the active DeskbarView instance.
                // One way: DeskbarView could register its BMessenger with App or a global service.
                BDeskbar db;
                BMessage updateMsg('VICN'); // View Icon Change
                updateMsg.AddString("image_path", path);
                // This is tricky: we need to send this to the *specific instance* of DeskbarView.
                // Deskbar::SendTo() might be an option if we know the item ID.
                // Or iterate through views in Deskbar's window.
                // For now, this is a placeholder for future implementation.
                 printf("Actual icon update in DeskbarView needs robust communication.\n");
            }
            break;
        }
        case 'TITL':
        {
            const char* new_title = NULL;
            if (message->FindString("title", &new_title) == B_OK) {
                printf("MainWindow::MessageReceived: TITL update - Title: %s\n", new_title);
                // TODO: Relay this to DeskbarView to update its tooltip (hover text).
                // Similar to icon update, needs communication with the DeskbarView instance.
                BMessage updateMsg('VTIL'); // View Title Change
                updateMsg.AddString("title", new_title);
                printf("Actual title update in DeskbarView needs robust communication.\n");
            }
            break;
        }
        case 'NOTI':
        {
            const char* msg_text = NULL;
            const char* notif_title = NULL;
            if (message->FindString("message", &msg_text) == B_OK &&
                message->FindString("notification_title", &notif_title) == B_OK) {
                printf("MainWindow::MessageReceived: NOTI - Title: %s, Message: %s\n", notif_title, msg_text);
                // Use BNotification for Haiku notifications
                BNotification notification(B_INFORMATION_NOTIFICATION);
                notification.SetGroup("Pystray"); // Group notifications from this app
                notification.SetTitle(notif_title);
                notification.SetContent(msg_text);
                // Optionally, set an icon for the notification
                // app_info info;
                // be_app->GetAppInfo(&info);
                // BBitmap appIcon(&info.ref, B_LARGE_ICON);
                // if (appIcon.InitCheck() == B_OK) {
                //    notification.SetIcon(&appIcon);
                // }
                // Optionally, set a message ID to identify the notification or handle clicks
                // notification.SetMessageID(...);
                notification.Send(); // Timeout is optional, default is used.
            }
            break;
        }
        case 'ABUT': // About message from DeskbarView context menu
            (new BAlert("About", "Pystray Haiku Helper\n\n"
                                 "Provides Deskbar integration for pystray.", "OK"))->Go(NULL);
            break;
        default:
            BWindow::MessageReceived(message);
            break;
    }
}

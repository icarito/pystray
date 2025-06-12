#include "App.h"
#include "MainWindow.h" // For creating and managing the main window
#include <Alert.h>     // For notifications or error messages
#include <Deskbar.h>   // For Deskbar interaction
#include <stdio.h>     // For printf (debugging)

const char* APP_SIGNATURE = "application/x-vnd.pystray-haiku-helper";

App::App() : BApplication(APP_SIGNATURE) {
    mainWindow = NULL;
    printf("App constructor: signature %s\n", APP_SIGNATURE);
}

App::~App() {
    printf("App destructor\n");
    // The mainWindow is a BWindow, it will be deleted by BApplication's Quit()
    // if it's still in the window list. If we remove it manually or it's closed,
    // it should have been deleted there. For this simple case, BApplication handles it.
}

void App::ReadyToRun() {
    printf("App::ReadyToRun()\n");
    // Create the main window. It will handle adding the replicant.
    // The window will be hidden, its purpose is to host the DeskbarView logic
    // and to be the target for messages.
    if (!mainWindow) {
        mainWindow = new MainWindow();
        // The MainWindow's constructor should handle showing itself or adding the replicant.
        // For a Deskbar app, the main window is usually hidden.
        // mainWindow->Show(); // Only if you need to see it for debugging
    }
}

bool App::QuitRequested() {
    printf("App::QuitRequested()\n");
    // Here, you could also remove the replicant from Deskbar if desired
    // BDeskbar deskbar;
    // if (deskbar.HasItem("PystrayDeskbarView")) {
    //     deskbar.RemoveItem("PystrayDeskbarView");
    // }
    return true;
}

void App::MessageReceived(BMessage* message) {
    printf("App::MessageReceived() what: %.4s\n", (char*)&message->what);
    switch (message->what) {
        // Example: handle a message to show/hide or update the replicant
        // case 'SHRG':
    // if (mainWindow) mainWindow->PostMessage(message, mainWindow); // Example
        //     break;
    case 'INIT':
        printf("App::MessageReceived: INIT message from pystray_init\n");
        if (!mainWindow) {
            mainWindow = new MainWindow();
            // MainWindow constructor now handles adding replicant and hiding.
        }
        break;
    case 'SHOW':
        printf("App::MessageReceived: SHOW message from pystray_show_icon\n");
        if (mainWindow) {
            // This would tell MainWindow to ensure the replicant is added/visible
            // For now, MainWindow adds it on creation. This could be a re-add or unhide.
            mainWindow->PostMessage(message, mainWindow);
        }
        break;
    case 'HIDE':
        printf("App::MessageReceived: HIDE message from pystray_hide_icon\n");
        if (mainWindow) {
            // This would tell MainWindow to remove/hide the replicant
            mainWindow->PostMessage(message, mainWindow);
        }
        break;
    case 'ICON':
    {
        printf("App::MessageReceived: ICON message from pystray_update_icon\n");
        const char* path = NULL;
        if (message->FindString("image_path", &path) == B_OK) {
            printf("Icon path: %s\n", path);
            // Pass to MainWindow/DeskbarView
            if (mainWindow) mainWindow->PostMessage(message, mainWindow);
        }
        break;
    }
    case 'TITL':
    {
        printf("App::MessageReceived: TITL message from pystray_update_title\n");
        const char* new_title = NULL;
        if (message->FindString("title", &new_title) == B_OK) {
            printf("New title: %s\n", new_title);
            // Pass to MainWindow/DeskbarView
            if (mainWindow) mainWindow->PostMessage(message, mainWindow);
        }
        break;
    }
    case 'NOTI':
    {
        printf("App::MessageReceived: NOTI message from pystray_notify\n");
        // Pass to MainWindow to display notification
        if (mainWindow) mainWindow->PostMessage(message, mainWindow);
        break;
    }
        default:
            BApplication::MessageReceived(message);
            break;
    }
}

int main() {
    printf("Starting PystrayHaikuHelper (main function)...\n");
    // In this C API model, main() might not be directly used if Python calls pystray_init() and pystray_run().
    // However, the BApplication instance is still needed.
    // Consider making 'app' a global pointer, initialized by pystray_init.
    // App app;
    // app.Run();
    // For now, let main create the app, but Python will drive it.
    // This means pystray_run() will run be_app->Run().
    if (be_app == NULL) { // Only create if not already done by pystray_init
        new App(); // The app instance is stored in be_app global
    }
    be_app->Run();
    printf("PystrayHaikuHelper main loop finished.\n");
    // delete be_app; // BApplication typically cleans itself up
    return 0;
}

// Global application pointer, managed by C API functions
// static App* gApp = NULL; // Replaced by be_app and be_app_messenger

// C API Implementations
extern "C" {

int pystray_init() {
    printf("C API: pystray_init() called\n");
    if (be_app) {
        printf("Application already initialized.\n");
        // Optionally, send a message to re-initialize or show the window/replicant
        // BMessage initMsg('INIT');
        // be_app_messenger.SendMessage(&initMsg);
        return 0; // Already initialized
    }
    // new App(); // Creates the BApplication instance, sets be_app
    // The MainWindow is created in App::ReadyToRun, which is called by BApplication::Run()
    // We need to ensure ReadyToRun logic (like creating MainWindow) happens.
    // One way is to post a message to self after app is created.
    // Or, call parts of ReadyToRun logic here.
    // For now, let's assume App constructor or a posted message handles MainWindow creation.

    // Create the BApplication. MainWindow will be created in App::ReadyToRun.
    // App::ReadyToRun is called when BApplication::Run() is called.
    // So, pystray_run() will trigger MainWindow creation.
    new App(); // Sets global be_app
    if (!be_app) {
        printf("pystray_init: Failed to create BApplication instance.\n");
        return -1; // Failure
    }

    // To ensure MainWindow and replicant are set up before Python might call other functions,
    // we can send a message that App::MessageReceived or MainWindow can handle.
    // Or, more directly, call the setup logic if be_app is ready.
    // For now, MainWindow creation is tied to App::ReadyToRun, triggered by pystray_run().
    // This means pystray_init() just sets up the app object.
    // If immediate UI setup is needed, post a message to trigger it.
    BMessenger(APP_SIGNATURE).SendMessage(new BMessage('INIT'));


    printf("pystray_init: BApplication instance created. Call pystray_run() to start event loop.\n");
    return 0; // Success
}

void pystray_run() {
    printf("C API: pystray_run() called\n");
    if (be_app) {
        be_app->Run(); // Starts the Haiku event loop
    } else {
        printf("pystray_run: Application not initialized. Call pystray_init() first.\n");
    }
}

void pystray_stop() {
    printf("C API: pystray_stop() called\n");
    if (be_app_messenger.IsValid()) {
        // Post a B_QUIT_REQUESTED message to the application.
        // This will go through the App::QuitRequested() flow.
        status_t err = be_app_messenger.SendMessage(B_QUIT_REQUESTED);
        if (err != B_OK) {
            printf("pystray_stop: Error sending B_QUIT_REQUESTED: %s\n", strerror(err));
        }
    } else {
        printf("pystray_stop: Application not running or not initialized.\n");
    }
}

void pystray_show_icon() {
    printf("C API: pystray_show_icon() called\n");
    if (be_app_messenger.IsValid()) {
        // This should message MainWindow to add/show the replicant
        BMessage msg('SHOW');
        be_app_messenger.SendMessage(&msg);
    } else {
        printf("pystray_show_icon: App not running.\n");
    }
}

void pystray_hide_icon() {
    printf("C API: pystray_hide_icon() called\n");
    if (be_app_messenger.IsValid()) {
        // This should message MainWindow to remove/hide the replicant
        BMessage msg('HIDE');
        be_app_messenger.SendMessage(&msg);
    } else {
        printf("pystray_hide_icon: App not running.\n");
    }
}

void pystray_update_icon(const char* image_path) {
    printf("C API: pystray_update_icon(%s) called\n", image_path ? image_path : "NULL");
    if (!image_path) return;
    if (be_app_messenger.IsValid()) {
        BMessage msg('ICON');
        msg.AddString("image_path", image_path);
        be_app_messenger.SendMessage(&msg);
    } else {
        printf("pystray_update_icon: App not running.\n");
    }
}

void pystray_update_title(const char* title) {
    printf("C API: pystray_update_title(%s) called\n", title ? title : "NULL");
    if (!title) return;
    if (be_app_messenger.IsValid()) {
        BMessage msg('TITL');
        msg.AddString("title", title);
        be_app_messenger.SendMessage(&msg);
    } else {
        printf("pystray_update_title: App not running.\n");
    }
}

void pystray_update_menu() {
    printf("C API: pystray_update_menu() called (Placeholder)\n");
    if (be_app_messenger.IsValid()) {
        // Placeholder: Send a message to rebuild the menu
        // BMessage msg('MENU');
        // be_app_messenger.SendMessage(&msg);
    } else {
        printf("pystray_update_menu: App not running.\n");
    }
}

void pystray_notify(const char* message, const char* notification_title) {
    printf("C API: pystray_notify(message: \"%s\", title: \"%s\") called\n",
           message ? message : "NULL", notification_title ? notification_title : "NULL");
    if (!message) message = ""; // Ensure not null
    if (!notification_title) notification_title = "Notification"; // Default title

    if (be_app_messenger.IsValid()) {
        BMessage msg('NOTI');
        msg.AddString("message", message);
        msg.AddString("notification_title", notification_title);
        be_app_messenger.SendMessage(&msg);
    } else {
        printf("pystray_notify: App not running.\n");
    }
}

void pystray_remove_notification() {
    printf("C API: pystray_remove_notification() called (Placeholder)\n");
    // Haiku notifications are typically transient.
    // Specific removal might not be directly possible or needed.
    // If future Haiku versions support it, a message could be sent.
}

} // extern "C"

#include "App.h"
#include "MainWindow.h" // For creating and managing the main window
#include <Alert.h>     // For notifications or error messages
#include <Deskbar.h>   // For Deskbar interaction
#include <stdio.h>     // For printf (debugging)
#include <string.h>    // For strerror
#include <ctype.h>     // For isprint used in msg_what_to_string_app

// Helper function to convert BMessage::what to a string for logging
// Using a more robust version similar to DeskbarView.cpp and MainWindow.cpp
static void msg_what_to_string_app(uint32 what, char* buffer, size_t buffer_size) {
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


const char* APP_SIGNATURE = "application/x-vnd.pystray-haiku-helper";

App::App() : BApplication(APP_SIGNATURE) {
    printf("App: Constructor called. Signature: %s\n", APP_SIGNATURE);
    mainWindow = NULL;
    printf("App: Constructor finished.\n");
}

App::~App() {
    printf("App: Destructor called.\n");
    // mainWindow (BWindow) is typically deleted by BApplication if in window list
}

void App::ReadyToRun() {
    printf("App: ReadyToRun called. Application is now ready to process events.\n");
    // MainWindow creation is now primarily driven by the 'INIT' message from pystray_init
    // to ensure it's created in a controlled manner after C API initialization.
    // A fallback could be added here if desired, but current design relies on C API.
    if (!mainWindow) {
        printf("App: ReadyToRun - mainWindow is NULL. It will be created upon 'INIT' message.\n");
    } else {
        printf("App: ReadyToRun - mainWindow already exists (unexpected before 'INIT' from C API).\n");
    }
    printf("App: ReadyToRun finished.\n");
}

bool App::QuitRequested() {
    printf("App: QuitRequested() called. Granting permission to quit.\n");
    // Any pre-quit cleanup for the app itself can go here.
    return true;
}

void App::MessageReceived(BMessage* message) {
    char whatStr[12];
    msg_what_to_string_app(message->what, whatStr, sizeof(whatStr)); // Use the app-specific helper
    printf("App: MessageReceived - what: %s\n", whatStr);
    // message->PrintToStream(); // Uncomment for full BMessage dump

    switch (message->what) {
    case 'INIT': // Sent from pystray_init
        printf("App: Message 'INIT' received. Action: Creating MainWindow if it doesn't exist.\n");
        if (!mainWindow) {
            printf("App: 'INIT' - MainWindow is NULL, creating new instance.\n");
            mainWindow = new MainWindow(); // Constructor handles replicant and hiding
        } else {
            printf("App: 'INIT' - MainWindow already exists. Forwarding 'INIT' to it for potential re-initialization.\n");
            mainWindow->PostMessage(message); // Forward to let MainWindow also handle it if needed
        }
        break;

    case B_QUIT_REQUESTED: // Can be sent by pystray_stop
        printf("App: Message B_QUIT_REQUESTED received. Passing to BApplication base to handle QuitRequested().\n");
        BApplication::MessageReceived(message); // Standard way to handle this
        break;

    // Messages from C API wrappers, to be forwarded to MainWindow
    case 'SHOW':
        printf("App: Message 'SHOW' (Show Icon) received from Python. Action: Forwarding to MainWindow.\n");
        if (mainWindow) mainWindow->PostMessage(message);
        else fprintf(stderr, "App: 'SHOW' - ERROR: mainWindow is NULL, cannot forward.\n");
        break;
    case 'HIDE':
        printf("App: Message 'HIDE' (Hide Icon) received from Python. Action: Forwarding to MainWindow.\n");
        if (mainWindow) mainWindow->PostMessage(message);
        else fprintf(stderr, "App: 'HIDE' - ERROR: mainWindow is NULL, cannot forward.\n");
        break;
    case 'ICON':
    {
        const char* path_info = "unknown";
        message->FindString("image_path", &path_info);
        printf("App: Message 'ICON' (Update Icon) received from Python. Path: '%s'. Action: Forwarding to MainWindow.\n", path_info ? path_info : "NOT_FOUND");
        if (mainWindow) mainWindow->PostMessage(message);
        else fprintf(stderr, "App: 'ICON' - ERROR: mainWindow is NULL, cannot forward.\n");
        break;
    }
    case 'TITL':
    {
        const char* title_info = "unknown";
        message->FindString("title", &title_info);
        printf("App: Message 'TITL' (Update Title) received from Python. Title: '%s'. Action: Forwarding to MainWindow.\n", title_info ? title_info : "NOT_FOUND");
        if (mainWindow) mainWindow->PostMessage(message);
        else fprintf(stderr, "App: 'TITL' - ERROR: mainWindow is NULL, cannot forward.\n");
        break;
    }
    case 'NOTI':
    {
        const char* msg_info = "unknown";
        const char* title_info = "unknown";
        message->FindString("message", &msg_info);
        message->FindString("notification_title", &title_info);
        printf("App: Message 'NOTI' (Notify) received from Python. Title: '%s', Msg: '%s'. Action: Forwarding to MainWindow.\n",
                title_info ? title_info : "NOT_FOUND", msg_info ? msg_info : "NOT_FOUND");
        if (mainWindow) mainWindow->PostMessage(message);
        else fprintf(stderr, "App: 'NOTI' - ERROR: mainWindow is NULL, cannot forward.\n");
        break;
    }
    // 'MENU' (pystray_update_menu) is a placeholder, no message handling yet.
    // 'RMNO' (pystray_remove_notification) is a placeholder, no message handling yet.

    default:
        printf("App: Message unhandled by App specific logic (what: %s), passing to BApplication::MessageReceived.\n", whatStr);
        BApplication::MessageReceived(message);
        break;
    }
    printf("App: MessageReceived - Finished processing what: %s.\n", whatStr);
}

int main() { // Kept for potential direct execution/testing
    printf("App: main() - Starting PystrayHaikuHelper application directly.\n");
    if (be_app == NULL) {
        printf("App: main() - No BApplication instance (be_app is NULL), creating new App().\n");
        new App();
    } else {
        printf("App: main() - BApplication instance (be_app) already exists (unexpected for direct run).\n");
    }

    if (be_app) {
        printf("App: main() - Calling be_app->Run(). This will block until quit.\n");
        be_app->Run();
        printf("App: main() - be_app->Run() returned. PystrayHaikuHelper main event loop finished.\n");
    } else {
        fprintf(stderr, "App: main() - ERROR: be_app is still NULL after new App(). Cannot run.\n");
        return 1;
    }
    return 0;
}

// C API Implementations
extern "C" {

int pystray_init() {
    printf("C API: pystray_init called from Python.\n");
    if (be_app) { // be_app is the global BApplication pointer
        printf("C API: pystray_init - Application (be_app) already seems to be initialized.\n");
        // Ensure MainWindow is created by sending 'INIT' message, even if app was already running.
        printf("C API: pystray_init - Posting 'INIT' message to potentially existing app instance.\n");
        status_t post_status = BMessenger(APP_SIGNATURE).SendMessage(new BMessage('INIT'));
         if (post_status != B_OK) {
            fprintf(stderr, "C API: pystray_init - WARN: Failed to post 'INIT' message to existing app: %s\n", strerror(post_status));
        }
        return 0;
    }

    printf("C API: pystray_init - Creating new App() instance.\n");
    new App(); // This sets the global be_app
    if (!be_app) {
        fprintf(stderr, "C API: pystray_init - ERROR: Failed to create BApplication instance (be_app is NULL after new App()).\n");
        return -1;
    }

    printf("C API: pystray_init - Posting 'INIT' message to new app instance to trigger MainWindow creation.\n");
    status_t post_status = BMessenger(APP_SIGNATURE).SendMessage(new BMessage('INIT'));
    if (post_status != B_OK) {
         fprintf(stderr, "C API: pystray_init - ERROR: Failed to post 'INIT' message to new app: %s. MainWindow might not be created.\n", strerror(post_status));
         // This is a potentially problematic state.
    }

    printf("C API: pystray_init - BApplication instance created. Python should call pystray_run() to start event loop.\n");
    return 0;
}

void pystray_run() {
    printf("C API: pystray_run called from Python.\n");
    if (be_app) {
        printf("C API: pystray_run - Calling BApplication::Run(). This is a blocking call.\n");
        be_app->Run();
        printf("C API: pystray_run - BApplication::Run() has returned (event loop finished).\n");
    } else {
        fprintf(stderr, "C API: pystray_run - ERROR: Application not initialized (be_app is NULL). Call pystray_init() first.\n");
    }
}

void pystray_stop() {
    printf("C API: pystray_stop called from Python.\n");
    if (be_app_messenger.IsValid()) { // be_app_messenger is a global BMessenger to be_app
        printf("C API: pystray_stop - Posting B_QUIT_REQUESTED to application via be_app_messenger.\n");
        status_t err = be_app_messenger.SendMessage(B_QUIT_REQUESTED);
        if (err != B_OK) {
            fprintf(stderr, "C API: pystray_stop - ERROR sending B_QUIT_REQUESTED: %s\n", strerror(err));
        }
    } else {
        fprintf(stderr, "C API: pystray_stop - ERROR: Application not running or not initialized (be_app_messenger invalid).\n");
    }
}

void pystray_show_icon() {
    printf("C API: pystray_show_icon called from Python.\n");
    if (be_app_messenger.IsValid()) {
        BMessage msg('SHOW');
        printf("C API: pystray_show_icon - Posting 'SHOW' message to App.\n");
        status_t err = be_app_messenger.SendMessage(&msg);
         if (err != B_OK) {
            fprintf(stderr, "C API: pystray_show_icon - ERROR sending 'SHOW' message: %s\n", strerror(err));
        }
    } else {
        fprintf(stderr, "C API: pystray_show_icon - ERROR: App not running (be_app_messenger invalid).\n");
    }
}

void pystray_hide_icon() {
    printf("C API: pystray_hide_icon called from Python.\n");
    if (be_app_messenger.IsValid()) {
        BMessage msg('HIDE');
        printf("C API: pystray_hide_icon - Posting 'HIDE' message to App.\n");
        status_t err = be_app_messenger.SendMessage(&msg);
        if (err != B_OK) {
            fprintf(stderr, "C API: pystray_hide_icon - ERROR sending 'HIDE' message: %s\n", strerror(err));
        }
    } else {
        fprintf(stderr, "C API: pystray_hide_icon - ERROR: App not running (be_app_messenger invalid).\n");
    }
}

void pystray_update_icon(const char* image_path) {
    printf("C API: pystray_update_icon called from Python with image_path: '%s'\n", image_path ? image_path : "NULL_PATH");
    if (!image_path) {
        fprintf(stderr, "C API: pystray_update_icon - ERROR: image_path argument is NULL.\n");
        return;
    }
    if (be_app_messenger.IsValid()) {
        BMessage msg('ICON');
        msg.AddString("image_path", image_path);
        printf("C API: pystray_update_icon - Posting 'ICON' message to App.\n");
        status_t err = be_app_messenger.SendMessage(&msg);
        if (err != B_OK) {
            fprintf(stderr, "C API: pystray_update_icon - ERROR sending 'ICON' message: %s\n", strerror(err));
        }
    } else {
        fprintf(stderr, "C API: pystray_update_icon - ERROR: App not running (be_app_messenger invalid).\n");
    }
}

void pystray_update_title(const char* title) {
    printf("C API: pystray_update_title called from Python with title: '%s'\n", title ? title : "NULL_TITLE");
    // Note: BMessage::AddString behavior with NULL might vary or be undesirable.
    // Python side sends empty string for NULL. If NULL could arrive here, handle explicitly.
    if (be_app_messenger.IsValid()) {
        BMessage msg('TITL');
        msg.AddString("title", title ? title : ""); // Ensure non-NULL if AddString requires it
        printf("C API: pystray_update_title - Posting 'TITL' message to App.\n");
        status_t err = be_app_messenger.SendMessage(&msg);
        if (err != B_OK) {
            fprintf(stderr, "C API: pystray_update_title - ERROR sending 'TITL' message: %s\n", strerror(err));
        }
    } else {
        fprintf(stderr, "C API: pystray_update_title - ERROR: App not running (be_app_messenger invalid).\n");
    }
}

void pystray_update_menu() {
    printf("C API: pystray_update_menu called from Python (Placeholder).\n");
    if (be_app_messenger.IsValid()) {
        // BMessage msg('MENU'); // Example if a message were used
        // printf("C API: pystray_update_menu - Posting 'MENU' message (placeholder).\n");
        // be_app_messenger.SendMessage(&msg);
        printf("C API: pystray_update_menu - No message sent as it's a placeholder on C++ side too.\n");
    } else {
        fprintf(stderr, "C API: pystray_update_menu - ERROR: App not running (be_app_messenger invalid).\n");
    }
}

void pystray_notify(const char* message, const char* notification_title) {
    printf("C API: pystray_notify called from Python. Title: \"%s\", Message: \"%s\"\n",
           notification_title ? notification_title : "NULL_TITLE", message ? message : "NULL_MSG");

    const char* msg_to_send = message ? message : ""; // Ensure non-NULL for BMessage
    const char* title_to_send = notification_title ? notification_title : "Notification"; // Default title

    if (be_app_messenger.IsValid()) {
        BMessage msg('NOTI');
        msg.AddString("message", msg_to_send);
        msg.AddString("notification_title", title_to_send);
        printf("C API: pystray_notify - Posting 'NOTI' message to App.\n");
        status_t err = be_app_messenger.SendMessage(&msg);
        if (err != B_OK) {
            fprintf(stderr, "C API: pystray_notify - ERROR sending 'NOTI' message: %s\n", strerror(err));
        }
    } else {
        fprintf(stderr, "C API: pystray_notify - ERROR: App not running (be_app_messenger invalid).\n");
    }
}

void pystray_remove_notification() {
    printf("C API: pystray_remove_notification called from Python (Placeholder).\n");
    // Currently no action. Haiku notifications are typically transient.
    // If a mechanism to remove them were added, it would be invoked here.
}

} // extern "C"

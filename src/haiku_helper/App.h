#ifndef APP_H
#define APP_H

#include <Application.h>
#include "MainWindow.h" // Forward declare or include for MainWindow pointer

extern const char* APP_SIGNATURE;

class App : public BApplication {
public:
    App();
    virtual ~App(); // Good practice to have a virtual destructor
    virtual void ReadyToRun();
    virtual bool QuitRequested();
    virtual void MessageReceived(BMessage* message); // To handle messages

private:
    MainWindow* mainWindow;
};

// C API for Python interaction
#ifdef __cplusplus
extern "C" {
#endif

int pystray_init();
void pystray_run();
void pystray_stop();
void pystray_show_icon(); // Might involve telling MainWindow to ensure replicant is there
void pystray_hide_icon(); // Might involve telling MainWindow to remove replicant
void pystray_update_icon(const char* image_path);
void pystray_update_title(const char* title);
void pystray_update_menu(); // Placeholder
void pystray_notify(const char* message, const char* notification_title);
void pystray_remove_notification(); // Placeholder

#ifdef __cplusplus
}
#endif

#endif // APP_H

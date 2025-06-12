#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <Window.h>

class DeskbarView; // Forward declaration

class MainWindow : public BWindow {
public:
    MainWindow();
    virtual ~MainWindow();

    virtual bool QuitRequested();
    virtual void MessageReceived(BMessage* message);

private:
    void _AddReplicantToDeskbar();
    void _RemoveReplicantFromDeskbar(); // For cleanup

    DeskbarView* fDeskbarView; // Instance of the view to be added to Deskbar
    bool fReplicantAdded;
    int32 fDeskbarReplicantID; // To store the ID from Deskbar
};

#endif // MAIN_WINDOW_H

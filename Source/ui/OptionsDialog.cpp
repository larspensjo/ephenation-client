// generated by Fast Light User Interface Designer (fluid) version 1.0300

#include "OptionsDialog.h"

void OptionsDialog::cb_fMusic_i(Fl_Check_Button*, void*) {
  gSoundControl.SwitchMusicStatus();
Options::sfSave.fMusicOn = fMusic->value();
}
void OptionsDialog::cb_fMusic(Fl_Check_Button* o, void* v) {
  ((OptionsDialog*)(o->parent()->parent()->parent()->user_data()))->cb_fMusic_i(o,v);
}

void OptionsDialog::cb_fPing_i(Fl_Check_Button*, void*) {
  gShowPing = fPing->value();
}
void OptionsDialog::cb_fPing(Fl_Check_Button* o, void* v) {
  ((OptionsDialog*)(o->parent()->parent()->parent()->user_data()))->cb_fPing_i(o,v);
}

void OptionsDialog::cb_Ok_i(Fl_Return_Button*, void*) {
  Options::sfSave.UpdateSettings(this);
fWindow->hide();
Fl::delete_widget(fWindow);
}
void OptionsDialog::cb_Ok(Fl_Return_Button* o, void* v) {
  ((OptionsDialog*)(o->parent()->user_data()))->cb_Ok_i(o,v);
}

void OptionsDialog::cb_Help_i(Fl_Button*, void*) {
  new HelpDialog;
}
void OptionsDialog::cb_Help(Fl_Button* o, void* v) {
  ((OptionsDialog*)(o->parent()->user_data()))->cb_Help_i(o,v);
}

void OptionsDialog::cb_Quit_i(Fl_Button*, void*) {
  fWindow->hide();
Fl::delete_widget(fWindow);
if (gMode.Get() == GameMode::GAME) {
	glfwCloseWindow();
} else {
	exit(0);
};
}
void OptionsDialog::cb_Quit(Fl_Button* o, void* v) {
  ((OptionsDialog*)(o->parent()->user_data()))->cb_Quit_i(o,v);
}

void OptionsDialog::cb_Cancel_i(Fl_Button*, void*) {
  fWindow->hide();
Fl::delete_widget(fWindow);
}
void OptionsDialog::cb_Cancel(Fl_Button* o, void* v) {
  ((OptionsDialog*)(o->parent()->user_data()))->cb_Cancel_i(o,v);
}

OptionsDialog::OptionsDialog() {
  { fWindow = new Fl_Double_Window(415, 365);
    fWindow->user_data((void*)(this));
    { Fl_Group* o = new Fl_Group(133, 5, 15, 15, "Options");
      o->labeltype(FL_ENGRAVED_LABEL);
      o->labelsize(20);
      o->align(Fl_Align(FL_ALIGN_RIGHT));
      o->end();
    } // Fl_Group* o
    { Fl_Tabs* o = new Fl_Tabs(25, 25, 360, 285);
      { Fl_Group* o = new Fl_Group(35, 45, 85, 265, "Miscellaneous");
        { fMusic = new Fl_Check_Button(35, 60, 70, 30, "Enable music");
          fMusic->tooltip("Enable or disable music.");
          fMusic->down_box(FL_DOWN_BOX);
          fMusic->callback((Fl_Callback*)cb_fMusic);
          fMusic->value(Options::sfSave.fMusicOn);
        } // Fl_Check_Button* fMusic
        { fPing = new Fl_Check_Button(35, 84, 70, 30, "Show ping");
          fPing->tooltip("Used to measure response time from server. This will add traffic, don\'t leav\
e it on for ever.");
          fPing->down_box(FL_DOWN_BOX);
          fPing->callback((Fl_Callback*)cb_fPing);
        } // Fl_Check_Button* fPing
        o->end();
      } // Fl_Group* o
      { fGraphicsTab = new Fl_Group(25, 50, 340, 250, "Graphics");
        fGraphicsTab->hide();
        fGraphicsTab->deactivate();
        { fPerformanceSlider = new Fl_Slider(55, 54, 45, 111, "Performance");
          fPerformanceSlider->tooltip("The general performance of your system. A high value will show better graphic\
s but at a lower FPS. If you change this value, it will override some other pa\
rameters.");
          fPerformanceSlider->minimum(1);
          fPerformanceSlider->maximum(4);
          fPerformanceSlider->step(1);
          fPerformanceSlider->value(1);
          fPerformanceSlider->slider_size(0.25);
          fPerformanceSlider->value(Options::sfSave.fPerformance);
        } // Fl_Slider* fPerformanceSlider
        { Fl_Group* o = new Fl_Group(110, 78, 15, 15, "Low");
          o->align(Fl_Align(FL_ALIGN_TOP_LEFT));
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(110, 105, 15, 15, "Medium");
          o->align(Fl_Align(FL_ALIGN_TOP_LEFT));
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(110, 132, 15, 15, "High");
          o->align(Fl_Align(FL_ALIGN_TOP_LEFT));
          o->end();
        } // Fl_Group* o
        { Fl_Group* o = new Fl_Group(110, 160, 15, 15, "Super");
          o->align(Fl_Align(FL_ALIGN_TOP_LEFT));
          o->end();
        } // Fl_Group* o
        { fGraphicMode = new Fl_Choice(85, 225, 105, 20, "Mode:");
          fGraphicMode->down_box(FL_BORDER_BOX);
        } // Fl_Choice* fGraphicMode
        { Fl_Group* o = new Fl_Group(105, 285, 15, 15, "Only change from start dialog");
          o->align(Fl_Align(FL_ALIGN_TOP_LEFT));
          o->end();
        } // Fl_Group* o
        { fFullScreenMode = new Fl_Check_Button(225, 225, 64, 15, "Full screen");
          fFullScreenMode->down_box(FL_DOWN_BOX);
        } // Fl_Check_Button* fFullScreenMode
        { fAmbientLight = new Fl_Value_Slider(200, 55, 150, 20, "Ambient light");
          fAmbientLight->tooltip("The amount of ambient light in dark areas.");
          fAmbientLight->type(1);
          fAmbientLight->maximum(100);
          fAmbientLight->step(1);
          fAmbientLight->value(25);
          fAmbientLight->slider_size(0.121212);
          fAmbientLight->textsize(14);
          fAmbientLight->value(Options::sfSave.fAmbientLight);
        } // Fl_Value_Slider* fAmbientLight
        { fSmoothTerrain = new Fl_Check_Button(215, 107, 64, 15, "Smooth terrain");
          fSmoothTerrain->tooltip("Show the world smooth instead of blocky. Depends on CPU performance, not grap\
hics.");
          fSmoothTerrain->down_box(FL_DOWN_BOX);
          fSmoothTerrain->value(Options::sfSave.fSmoothTerrain);
        } // Fl_Check_Button* fSmoothTerrain
        { fMergeNormals = new Fl_Check_Button(215, 132, 64, 15, "Merge normals");
          fMergeNormals->tooltip("If showing the world as smooth, this option will enhance it further. Depends \
on CPU, not on graphics.");
          fMergeNormals->down_box(FL_DOWN_BOX);
          fMergeNormals->value(Options::sfSave.fMergeNormals);
        } // Fl_Check_Button* fMergeNormals
        { fAddNoise = new Fl_Check_Button(215, 157, 64, 15, "Add noise to terrain");
          fAddNoise->tooltip("If showing the world as smooth, this option will add some random height to ge\
t variation. Depends on CPU performance.");
          fAddNoise->down_box(FL_DOWN_BOX);
          fAddNoise->value(Options::sfSave.fAddNoise);
        } // Fl_Check_Button* fAddNoise
        { fDynamicShadows = new Fl_Check_Button(215, 182, 64, 15, "Dynamic shadows");
          fDynamicShadows->tooltip("Draw realistic shadows.");
          fDynamicShadows->down_box(FL_DOWN_BOX);
          fDynamicShadows->value(Options::sfSave.fDynamicShadows);
        } // Fl_Check_Button* fDynamicShadows
        fGraphicsTab->end();
      } // Fl_Group* fGraphicsTab
      o->end();
    } // Fl_Tabs* o
    { Fl_Return_Button* o = new Fl_Return_Button(30, 320, 72, 20, "Ok");
      o->callback((Fl_Callback*)cb_Ok);
    } // Fl_Return_Button* o
    { Fl_Button* o = new Fl_Button(121, 320, 70, 20, "Help...");
      o->callback((Fl_Callback*)cb_Help);
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(202, 320, 90, 20, "Quit game");
      o->callback((Fl_Callback*)cb_Quit);
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(305, 320, 64, 20, "Cancel");
      o->callback((Fl_Callback*)cb_Cancel);
    } // Fl_Button* o
    fWindow->set_modal();
    fWindow->end();
  } // Fl_Double_Window* fWindow
  fWindow->show();
}

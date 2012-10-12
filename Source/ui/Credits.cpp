// generated by Fast Light User Interface Designer (fluid) version 1.0110

#include "Credits.h"

void CreditScreen::cb_Close_i(Fl_Return_Button*, void*) {
  fWindow->hide();
Fl::delete_widget(fWindow);
}
void CreditScreen::cb_Close(Fl_Return_Button* o, void* v) {
  ((CreditScreen*)(o->parent()->user_data()))->cb_Close_i(o,v);
}

CreditScreen::CreditScreen() {
  { fWindow = new Fl_Window(475, 510, "Ephenation credits");
    fWindow->user_data((void*)(this));
    { fOutput = new Fl_Output(5, 25, 460, 415);
      fOutput->type(12);
    } // Fl_Output* fOutput
    { Fl_Return_Button* o = new Fl_Return_Button(35, 470, 72, 20, "Close");
      o->callback((Fl_Callback*)cb_Close);
    } // Fl_Return_Button* o
    fWindow->set_modal();
    fWindow->end();
  } // Fl_Window* fWindow
}

void CreditScreen::Show() {
  const char *msg = "Lead programmers:\n"
"\tLars Pensj�\n"
"\tMikael Grah\n"
"\nLevel design and testing: Andreas Andersson\n"
"\nMusic: Kevin MacLeod, http://incompetech.com\n"
"\nSund effects: http://www.mediacollege.com/\n"
"\nGraphics support: Jonas Pensj�, David Condon\n"
"\nSource code inspiration:\n"
"\thttp://www.videotutorialsrock.com\n"
"\thttp://www.lighthouse3d.com\n"
"\tSimplex noise by Stefan Gustavson\n"
"\nLibraries:\n"
"  glm:     G-Truc Creation (www.g-truc.net)\n"
"  glfw:    http://www.glfw.org/\n"
"  tinyxml: Lee Thomason, www.sourceforge.net/projects/tinyxml\n"
;
fOutput->value(msg);
fWindow->show();
}
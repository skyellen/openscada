#include "tapplication.h"

TApplication *App;

int main(int argc, char *argv[], char *envp[] )
{
//    while(*envp) printf("%s\n",*envp++);
    
    App = new TApplication(argc,argv);
    int rez = App->run();
    delete App;

    return(rez);
}


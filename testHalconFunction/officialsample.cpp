#include <ctime>
#include "HalconCpp.h"

  int main()
  {
    using namespace Halcon;

    HImage  Mandrill("monkey");          // read image from file "monkey"
	//clock_t start = clock();
 //   HImage  Mandrill("defect_1.bmp");          // read image from file "monkey"
	//printf("read image time:%d.", clock()-start);
    HWindow w;                           // window with size equal to image

    Mandrill.Display(w);                 // display image in window
    w.Click();                           // wait for mouse click

    HRegion Bright = Mandrill >= 128;    // select all bright pixels
    HRegionArray Conn = Bright.Connection(); // get connected components

    // select regions with a size of at least 500 pixels
    HRegionArray Large = Conn.SelectShape("area","and",500,90000); 

    // select the eyes out of the instance variable Large by using 
    // the anisometry as region feature:
    HRegionArray Eyes = Large.SelectShape("anisometry","and",1,1.7);

	w.ClearWindow();
    Eyes.Display(w);                     // display result image in window
    w.Click();                           // wait for mouse click 
	system("pause");
	return 0;
  }
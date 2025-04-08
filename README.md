# saleae_dmx_analyzers
Modified DMX512 analyzer dlls which accept high speed transmissions/multi universe 

-----

The built in DMX512 analyzer in Logic/Logic2 can only handle standard DMX512 transmissions speeds (250kHz).

These modified analyzers allow proper viewing of Break, MAB, start codes and slots at the following transmission speeds;
* 250kHz (Standard - 1 Universe at 44Hz)
* 500kHz (2 Universe at 44Hz)
* 750kHz (3 Universe at 44Hz)
* 1000kHz (4 Universe at 44Hz)
* 1250kHz (5 Universe at 44Hz)
* 1500kHz (6 Universe at 44Hz)

  -----

  ## Usage
  These analyzers are compiled for the windows(x64) version of Logic2. I may get round to compiling for the other OS Logic supports.
  
  Simply download the .dll file you require for the transmission speed you're using and place in;
  
  **C:\Program Files\Logic\Resources\windows-x64\Analyzers**
  

Den här filen beskriver lite kort vad som behövs för att kompilera och köra
programmet på Raspberry Pin.

 ---------
|Bibliotek|
 ---------
De bibliotek som behövs är OpenCV, Userland och raspicam_cv. OpenCV finns att
laddas ner på http://opencv.org och Userland på https://github.com/raspberrypi/userland.
Biblioteket raspicam_cv finns redan nedladdat och inlagt i mappen libs, så
detta behöver inte laddas ner.

Dessa filer är redan korrekt länkade på Raspberry Pin, men om konfigurationen
skulle ändras, tex formatering eller en ny Pi, ska filen CMakeLists.txt uppdateras
så att de relevanta sökvägarna där pekar på mapparna biblioteken ligger i.

 ---------------------
|Kompilering & körning|
 ---------------------
Först måste en makefile skapas genom Cmake. Detta behöver endast göras om
CMakeLists.txt ändrats sedan den senaste makefilen skapades. För att skapa en
makefile, öppna en terminal och navigera till källmappen för projektet (som denna
fil ligger i om den inte flyttats) och kör kommandot "cmake .". Om CMakeLists.txt
är rätt konfigurerad kommer en makefile att skapas. Därefter ska kommandot "make"
köras, och om kompileringen gått igenom korrekt kan programmet startas genom att
köra "./robot". Namnet på det kompilerade programmet kan ändras i CMakeLists.txt
genom att ändra project( robot ) på rad 2 till ett annat namn.
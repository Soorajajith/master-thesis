Den h�r filen beskriver lite kort vad som beh�vs f�r att kompilera och k�ra
programmet p� Raspberry Pin.

 ---------
|Bibliotek|
 ---------
De bibliotek som beh�vs �r OpenCV, Userland och raspicam_cv. OpenCV finns att
laddas ner p� http://opencv.org och Userland p� https://github.com/raspberrypi/userland.
Biblioteket raspicam_cv finns redan nedladdat och inlagt i mappen libs, s�
detta beh�ver inte laddas ner.

Dessa filer �r redan korrekt l�nkade p� Raspberry Pin, men om konfigurationen
skulle �ndras, tex formatering eller en ny Pi, ska filen CMakeLists.txt uppdateras
s� att de relevanta s�kv�garna d�r pekar p� mapparna biblioteken ligger i.

 ---------------------
|Kompilering & k�rning|
 ---------------------
F�rst m�ste en makefile skapas genom Cmake. Detta beh�ver endast g�ras om
CMakeLists.txt �ndrats sedan den senaste makefilen skapades. F�r att skapa en
makefile, �ppna en terminal och navigera till k�llmappen f�r projektet (som denna
fil ligger i om den inte flyttats) och k�r kommandot "cmake .". Om CMakeLists.txt
�r r�tt konfigurerad kommer en makefile att skapas. D�refter ska kommandot "make"
k�ras, och om kompileringen g�tt igenom korrekt kan programmet startas genom att
k�ra "./robot". Namnet p� det kompilerade programmet kan �ndras i CMakeLists.txt
genom att �ndra project( robot ) p� rad 2 till ett annat namn.
This folder contains everything that has to do with the network.

The client software that is used to control the robot can start a program called prediction.exe.
The program PyInstaller is used to make prediction.py an executable.
PyInstaller generates a folder that needs to be moved to the same location as the client software for this to be able to work.
prediction.py can of course be started manually from this folder, the executable is just for convenience.

To generate the executable the following must be passed to PyInstaller:
	
	pyinstaller --add-data model.h5;. --hidden-import=h5py --hidden-import=h5py.defs --hidden-import=h5py.utils --hidden-import=h5py.h5ac --hidden-import=h5py._proxy prediction.py
	
Be aware of the included network model. Either rename the saved model, or modifie fine_tune.py.
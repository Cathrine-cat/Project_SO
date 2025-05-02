```text
TREASURE MANAGER
Application for managing treasure hunts

Overall structure of files :
Example:
-root folder:
	-treasure_manager.c
	-hunts:
		-hunt1:
			-treasure.bin
			-hunt1.log
		-hunt2: ...
		...
		-general.log
	-logged_hunt-hunt1
	-logged_hunt-hunt2
	...
(Symoblic link: logged_hunt-hunt1 -> hunt1.log)


Required files:
Only the treasure_manager.c is required. The program creates the hunts itself, if they are not existent, during the add treasure operation. The user can create the hunts by specifying an ID that is valid - that follows a specific pattern.
(By default that pattern is hunt[number]).

Operations:
-Add treasure: adds to a specified hunt, identified by hunt_id (a string that follows a pattern) a treasure, identified by an ID.
The IDs of the treasures are numbers that start from 1. Each treasure in a specific hunt is unique and their IDs are generates such that they are consecutive numbers.
This function allows the user to introduce data for a new treasure into the treasure file. The data is being checked and errors are detected, not allowing incorrect data to be entered. 
It also offers the user the possibility to re-enter data if it is not valid and check again the data before committing to it.

-List hunt: list information about a hunt, identified by hunt_id.
The size of the hunt directory is calculated by parsing the directory.
The treasures are listed by ID.

-View treasure: view details of a specific treasure. 

-Remove treasure: removes a treasure for a specified hunt. 
This function re-indexes the IDs of the treasures, such that they are still consecutive numbers starting from one and there are no gaps in the file.

-Remove hunt: removes a hunt directory. 
This deletes the treasure file and the log file. It also removes the symbolic link.

-Log operation: logs every operation performed. 
This function creates a log file. It can create a log file for a specific hunt inside that directory and a symbolic link to it in the root. There it writes all operations performed on a specific hunt.
It can also create and write into a general logs file, that tracks all operations performed, including the creation and deletion of hunts.
```
``

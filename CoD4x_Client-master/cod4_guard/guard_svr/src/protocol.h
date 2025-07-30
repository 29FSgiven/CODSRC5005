/*

    Frame header:
        4 bytes         4 bytes           n bytes
    -----------------------------------------------------
    |  Frame size  |  Command type  |  Command Payload  |       
    -----------------------------------------------------

*/
#define DEFAULT_FEATURES_VERSION 1

enum class Features
{
    FEOF = 0, //No more features
    NOLOGIN = 1, //No login required
    LOGINPLAIN = 2,
    STARTTLS = 10, //StartTLS is supported for login
    STARTTLSFULL = 11, //StartTLS is supported for full session
    MAXFEATURES = 12
};

enum class CommandType
{
    HELO = 0,
    HELORESPONSE = 1,
    FEATURES = 2,
    STARTTLS = 3,
    GETDIRINFO = 4,
    DIRINFORESPONSE = 5,
    GETFILE = 6,
    GETFILERESPONSE = 7,
    GETCHUNKS = 8,
    GETCHUNKSRESPONSE = 9,
    SERVERERROR = 10,
    TLSSWITCH = 11
};

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define THISCLIENTNAME "FileSyncClientv1.0"


/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++

                 Command Payload

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Handshake commands:
------------------------------------------------------------------------
--- Client handshake sent to server: HELO = 0 --------------------------

Frame header (Client --> Server): 
      4 bytes        4 bytes
-----------------------------------
|  Major version |  Minor version |
-----------------------------------


------------------------------------------------------------------------
--- Server handshake sent to client: HELORESPONSE = 1 ------------------

Frame header (Server --> Client): 
      4 bytes        4 bytes
-----------------------------------
|  Major version |  Minor version |
-----------------------------------

------------------------------------------------------------------------
--- Feature list sent from server to client after HELO: FEATURES = 2 ---

Frame header (Server --> Client): 
    4 bytes          4 bytes       4 bytes    4 bytes          4 bytes
-------------------------------------------------------------------------
| feature type | feature version | options1 | options2 | ... | optionsN |   repeats until a -1 option header is sent
-------------------------------------------------------------------------

Feature headers getting appended together. Final header:
    4 bytes
--------------------
| feature type = 0 |
--------------------



Dirinfo commands:
-----------------------------------------------
--- Request directory list: GETDIRINFO  = 4 ---

Frame header (Client --> Server): 
      n bytes
---------------------
|  directory name   |
---------------------

---------------------------------------------------------
--- Response for directory query: DIRINFORESPONSE = 5 ---

Frame header (Server --> Client):
    4 bytes          n bytes       8 bytes           32 bytes          n bytes       8 bytes           32 bytes                        n bytes        8 bytes         32 bytes
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
| number of files |  filename1  |  filesize1   |    Blake2s hash1   |  filename2  |   filesize2   |   Blake2s hash2   |    ...   |    filenameN   |   filesizeN  |  Blake2s hashN |
-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------



Download commands:
-----------------------------------------
--- Request filedownload: GETFILE = 6 ---

Frame header (Client --> Server):
    n bytes     n bytes
--------------------------
|  directory |  filename |
--------------------------

------------------------------------------------------------
--- Response for filedownload query: GETFILERESPONSE = 7 ---

Frame header (Server --> Client):
   n bytes       8 bytes         32 bytes       
------------------------------------------------
|  filename  |  filesize   |    Blake2s hash   |
------------------------------------------------


----------------------------------------------------
--- Request to begin filedownload: GETCHUNKS = 8 ---

Frame header (Client to Server): 
     8 bytes       
-------------------
|   fileoffest    | (Ignored for now)
-------------------

-----------------------------------------------------
--- Response with filedata: GETCHUNKSRESPONSE = 9 ---


Frame header (Server --> Client):
    4 bytes      
-----------------
|  chunkheader  |
-----------------     4 bytes         8 bytes           chunksize bytes
                 ------------------------------------------------------  
    if 1 then    |   chunksize   |    fileoffset     |    filedata    |
                 ------------------------------------------------------

    else if 0 then restart filesync

    Case 1 repeats until chunksize = 0
*/
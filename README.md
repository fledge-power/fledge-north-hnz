HNZ C/C++ North plugin
===============================================================================

A simple asynchronous HNZ plugin that pulls data from Fledge and sends
it to a client.

To build this plugin, you will need the HNZ library installed on your environment
as described below.

You also need to have Fledge installed from the source code, not from the 
package repository.


Building HNZ lib
-----------------

To build HNZ C/C++ North plugin, you need to have the HNZ library and then:

.. code-block:: console

  $ cd libhnz/
  $ export LIB_HNZ=`pwd`

As shown above, you need a $LIB_HNZ env var set to the source tree of the
library.

Then, you can build the library with:

.. code-block:: console

  $ cd libhnz/src/hnz
  $ ./compilation.sh



Build
-----


To build the hnz plugin, once you are in the plugin source tree you need to run:

.. code-block:: console

  $ mkdir build
  $ cd build
  $ cmake ..
  $ make

- By default the Fledge develop package header files and libraries
  are expected to be located in /usr/include/fledge and /usr/lib/fledge
- If **FLEDGE_ROOT** env var is set and no -D options are set,
  the header files and libraries paths are pulled from the ones under the
  FLEDGE_ROOT directory.
  Please note that you must first run 'make' in the FLEDGE_ROOT directory.

You may also pass one or more of the following options to cmake to override 
this default behaviour:

- **FLEDGE_SRC** sets the path of a Fledge source tree
- **FLEDGE_INCLUDE** sets the path to Fledge header files
- **FLEDGE_LIB sets** the path to Fledge libraries
- **FLEDGE_INSTALL** sets the installation path of the plugin

NOTE:
 - The **FLEDGE_INCLUDE** option should point to a location where all the Fledge 
   header files have been installed in a single directory.
 - The **FLEDGE_LIB** option should point to a location where all the Fledge
   libraries have been installed in a single directory.
 - 'make install' target is defined only when **FLEDGE_INSTALL** is set

Examples:

- no options

  $ cmake ..

- no options and FLEDGE_ROOT set

  $ export FLEDGE_ROOT=/some_fledge_setup

  $ cmake ..

- set FLEDGE_SRC

  $ cmake -DFLEDGE_SRC=/home/source/develop/Fledge  ..

- set FLEDGE_INCLUDE

  $ cmake -DFLEDGE_INCLUDE=/dev-package/include ..
- set FLEDGE_LIB

  $ cmake -DFLEDGE_LIB=/home/dev/package/lib ..
- set FLEDGE_INSTALL

  $ cmake -DFLEDGE_INSTALL=/home/source/develop/Fledge ..

  $ cmake -DFLEDGE_INSTALL=/usr/local/fledge ..


Using the plugin
----------------

As described in the Fledge documentation, you can use the plugin by adding 
a service from a terminal, or from the web API.

1 - Add the service from a terminal:

.. code-block:: console

  $ curl -sX POST http://localhost:8081/fledge/service -d '{"name":"hnz_name","type":"north","plugin":"hnz","enabled":true}'

Or

2) Add the service from the web API:

 - On the web API, go to the North tab
 - Click on "Add +"
 - Select hnz and give it a name, then click on "Next"
 - Change the default settings to your settings, then click on "Next"
 - Let the "Enabled" option checked, then click on "Done"


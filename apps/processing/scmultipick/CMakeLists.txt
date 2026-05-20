SET(PICK_TARGET scmultipick)

SET(
	PICK_SOURCES
		main.cpp
		app.cpp
		detector.cpp
		phaseclassifier.cpp
		config.cpp
		stationconfig.cpp
)

SET(
	PICK_HEADERS
		picker.h
		detector.h
		phaseclassifier.h
		config.h
		stationconfig.h
)

SET(
	PICK_DATA
		config/station.conf
		config/scmultipick.cfg
)

INCLUDE_DIRECTORIES(.)

SC_ADD_EXECUTABLE(PICK ${PICK_TARGET})
SC_LINK_LIBRARIES_INTERNAL(${PICK_TARGET} client)
SC_INSTALL_DATA(PICK ${PICK_TARGET})
SC_INSTALL_INIT(${PICK_TARGET} ${INIT_TEMPLATE})

FILE(GLOB descs "${CMAKE_CURRENT_SOURCE_DIR}/descriptions/*.xml")
INSTALL(FILES ${descs} DESTINATION ${SC3_PACKAGE_APP_DESC_DIR})

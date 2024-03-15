project(nigogame LANGUAGES CXX)
if (WIN32)

	if(CMAKE_BUILD_TYPE STREQUAL "Debug")
		add_executable(nigogame
			nigogame.cpp 
			game/game.cpp
			game/main_menu_scene.cpp
			game/game_scene.cpp
			game/player.cpp
			game/tilemap.cpp
			game/actors.cpp
			game/moco.cpp
		)
	else()
		add_executable(nigogame WIN32
			nigogame.cpp 
			game/game.cpp
			game/main_menu_scene.cpp
			game/game_scene.cpp
			game/player.cpp
			game/tilemap.cpp
			game/actors.cpp
			game/moco.cpp
		)
	endif()
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup")
else()
	add_executable(nigogame 
		nigogame.cpp 
		game/game.cpp
		game/main_menu_scene.cpp
		game/game_scene.cpp
		game/player.cpp
		game/tilemap.cpp
		game/actors.cpp
		game/moco.cpp
	)
endif()
target_link_libraries(nigogame PRIVATE nk)

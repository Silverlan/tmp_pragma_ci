/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Silverlan
 */

#define MUTIL_STATIC

#ifdef _WIN32
#include <windows.h>
#endif

#include <iostream>
#include <string>
#include <filesystem>
#include <sharedutils/util.h>
#include <sharedutils/util_path.hpp>

static int update_failed()
{
	std::cout << "Press any key to exit..." << std::endl;
	std::cin.get();
	return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
	if(argc == 0)
		return EXIT_FAILURE;
	std::string path = argv[0];

	std::string pragmaExecutableName;
#ifdef _WIN32
	pragmaExecutableName = "pragma.exe";
#else
	pragmaExecutableName = "pragma";
#endif
	std::cout << "Waiting for pragma to close..." << std::endl;
	// Wait until pragma has been closed
	while(util::is_process_running(pragmaExecutableName.c_str()))
		std::this_thread::sleep_for(std::chrono::milliseconds(250));

	auto pos = path.find_last_of("\\/");
	if(pos != std::string::npos)
		path = path.substr(0, pos);

	pos = path.find_last_of("\\/");
	if(pos != std::string::npos)
		path = path.substr(0, pos);

	char pathSeparator;
#ifdef _WIN32
	pathSeparator = '\\';
#else
	pathSeparator = '/';
#endif

	std::string updatePath = path + pathSeparator + "update";
	if(!std::filesystem::is_directory(updatePath)) {
		std::cout << "Unable to find update folder '" << updatePath << "'!" << std::endl;
		return update_failed();
	}

	auto updaterPath = updatePath + pathSeparator + "bin" + pathSeparator;
#ifdef _WIN32
	updaterPath += "updater.exe";
#else
	updaterPath += "updater";
#endif
	if(std::filesystem::exists(updaterPath)) {
		std::cout << "Deleting '" << updaterPath << "'..." << std::endl;
		std::filesystem::remove(updaterPath);
	}

	// Copy files from update folder to root folder
	auto pUpdatePath = util::Path::CreatePath(updatePath);
	for(const auto &entry : std::filesystem::recursive_directory_iterator(updatePath)) {
		if(std::filesystem::is_regular_file(entry)) {
			auto fullSrcPath = entry.path().string();
			auto relPath = util::Path::CreateFile(fullSrcPath);
			relPath.MakeRelative(pUpdatePath);

			auto filename = relPath.GetString();
			auto newPath = path + pathSeparator + filename;
			std::cout << "Copying '" << relPath << "'..." << std::endl;
			auto success = true;
			try {
				std::filesystem::create_directories(ufile::get_path_from_filename(newPath));
				success = std::filesystem::copy_file(fullSrcPath, newPath, std::filesystem::copy_options::overwrite_existing);
			}
			catch(const std::filesystem::filesystem_error &err) {
				std::cout << "Failed to copy file '" << relPath << "': " << err.what() << "!" << std::endl;
				return update_failed();
			}
			if(success == false) {
				std::cout << "Failed to copy file '" << relPath << "'!" << std::endl;
				return update_failed();
			}
		}
	}

	// Delete "update" directory
	std::cout << "Deleting '" << updatePath << "'..." << std::endl;
	std::filesystem::remove_all(updatePath);

	return EXIT_SUCCESS;
}

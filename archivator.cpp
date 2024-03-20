#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

struct FileInfo {
    std::string filename;
    std::string path;
    std::uintmax_t size;
};

void archiveFiles(const std::string& folderPath, const std::string& archiveName);
void unarchiveFiles(const std::string& archiveName, const std::string& targetFolder);
bool checkArchive(const std::string& archiveName, const std::string& folderPath);

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <-a/-u> <folder/archive> <output_folder/archive>" << '\n';
        return 1;
    }

    std::string mode = argv[1];
    std::string source = argv[2];
    std::string destination = argv[3];

    if (mode == "-a") {
        archiveFiles(source, destination);
    } else if (mode == "-u") {
        unarchiveFiles(source, destination);
    } else {
        std::cerr << "Invalid mode. Please use -a for archiving or -u for unarchiving." << '\n';
        return 1;
    }

    return 0;
}

void archiveFiles(const std::string& folderPath, const std::string& archiveName) {
    if (checkArchive(archiveName, folderPath)) {
        std::cout << "Archive already exists and contains identical files. Skipping archiving." << '\n';
        return;
    }
    
    std::ofstream archive(archiveName, std::ios::binary);
    if (!archive.is_open()) {
        std::cerr << "Failed to open archive for writing." << '\n';
        return;
    }

    std::vector<FileInfo> files;

    // Collect file information
    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (fs::is_regular_file(entry)) {
            FileInfo fileInfo;
            fileInfo.filename = entry.path().filename().string();
            fileInfo.path = entry.path().string();
            fileInfo.size = fs::file_size(entry);
            files.push_back(fileInfo);
        }
    }

    // Write header and file contents
    for (const auto& fileInfo : files) {
        // Write header
        archive << fileInfo.filename << '\0'; // Using '\0' as delimiter
        archive.write(reinterpret_cast<const char*>(&fileInfo.size), sizeof(std::uintmax_t));

        // Write file contents
        std::ifstream inputFile(fileInfo.path, std::ios::binary);
        if (!inputFile.is_open()) {
            std::cerr << "Failed to open file: " << fileInfo.filename << '\n';
            continue;
        }
        archive << inputFile.rdbuf();
        inputFile.close();
    }

    archive.close();
    std::cout << "Archiving complete." << '\n';
}

void unarchiveFiles(const std::string& archiveName, const std::string& targetFolder) {
    std::ifstream archive(archiveName, std::ios::binary);
    if (!archive.is_open()) {
        std::cerr << "Failed to open archive for reading." << '\n';
        return;
    }

    // Create directory for unarchived files
    fs::create_directories(targetFolder);

    while (!archive.eof()) {
        FileInfo fileInfo;
        std::getline(archive, fileInfo.filename, '\0');
        if (fileInfo.filename.empty()) // Check for end of header
            break;
        archive.read(reinterpret_cast<char*>(&fileInfo.size), sizeof(std::uintmax_t));

        std::ofstream outputFile(targetFolder + "/" + fileInfo.filename, std::ios::binary);
        if (!outputFile.is_open()) {
            std::cerr << "Failed to create file: " << fileInfo.filename << '\n';
            continue;
        }

        char* buffer = new char[fileInfo.size];
        archive.read(buffer, fileInfo.size);
        outputFile.write(buffer, fileInfo.size);
        delete[] buffer;
    }

    archive.close();
    std::cout << "Unarchiving complete." << '\n';
}

bool checkArchive(const std::string& archiveName, const std::string& folderPath) {
    std::ifstream archive(archiveName, std::ios::binary);
    if (!archive.is_open()) {
        return false; // Archive doesn't exist
    }

    // // Read headers from archive
    std::vector<FileInfo> archivedFiles;
     while (!archive.eof()) {
        FileInfo fileInfo;
        std::getline(archive, fileInfo.filename, '\0');
        if (fileInfo.filename.empty()) // Check for end of header
            break;
        archive.read(reinterpret_cast<char*>(&fileInfo.size), sizeof(std::uintmax_t));
        archivedFiles.push_back(fileInfo);

        std::string folderFilePath = folderPath + "/" + fileInfo.filename;
        std::ifstream folderFile(folderFilePath, std::ios::binary);
        if (!fs::exists(folderFilePath) || fs::file_size(folderFilePath) != fileInfo.size) {
            archive.close();
            folderFile.close();
            return false; // File not found or size mismatch
        }

        char* bufferArchive = new char[fileInfo.size];
        char* bufferFile = new char[fs::file_size(folderFilePath)];
        archive.read(bufferArchive, fileInfo.size);
        folderFile.read(bufferFile, fs::file_size(folderFilePath));
        if (strcmp(bufferArchive, bufferFile) != 0) {
            folderFile.close();
            archive.close();
            delete[] bufferArchive;
            delete[] bufferFile;
            return false; // File content mismatch
        }
        delete[] bufferArchive;
        delete[] bufferFile;
        folderFile.close();
    }
    archive.close();

    // Iterate through folder files to compare with archive number of files
    int counterOfFiles = 0;
    for (const auto& entry : fs::recursive_directory_iterator(folderPath)) {
        if (fs::is_regular_file(entry)) {
            counterOfFiles++;
        }
    }
    if (counterOfFiles != (int)archivedFiles.size()) return false; //Number of files is diferent

    return true; // Archive is identical
}


//
//  utils.cpp
//
//  Copyright 2020 Marco Oliva. All rights reserved.
//


#include <utils.hpp>


//------------------------------------------------------------------------------

const std::string vcfbwt::TempFile::DEFAULT_TEMP_DIR = ".";
std::string vcfbwt::TempFile::temp_dir = vcfbwt::TempFile::DEFAULT_TEMP_DIR;

// By storing the filenames in a static object, we can delete the remaining
// temporary files when std::exit() is called.
struct Handler
{
    std::mutex tempfile_lock;
    std::size_t counter;
    std::set<std::string> filenames;
    
    Handler() : counter(0) {}
    
    ~Handler()
    {
        std::lock_guard<std::mutex>(this->tempfile_lock);
        for(auto& filename : this->filenames)
        {
            std::remove(filename.c_str());
        }
        this->filenames.clear();
    }
} handler;

void
vcfbwt::TempFile::setDirectory(const std::string& directory)
{
    std::lock_guard<std::mutex>(handler.tempfile_lock);
    if(directory.empty()) { temp_dir = DEFAULT_TEMP_DIR; }
    else if(directory[directory.length() - 1] != '/') { temp_dir = directory; }
    else { temp_dir = directory.substr(0, directory.length() - 1); }
}

std::string
vcfbwt::TempFile::getName(const std::string& name_part)
{
    char hostname[32];
    gethostname(hostname, 32); hostname[31] = 0;
    
    std::string filename;
    {
        std::lock_guard<std::mutex>(handler.tempfile_lock);
        filename = temp_dir + '/' + name_part + '_'
        + std::string(hostname) + '_'
        + std::to_string(pid()) + '_'
        + std::to_string(handler.counter);
        handler.filenames.insert(filename);
        handler.counter++;
    }
    
    return filename;
}

void
vcfbwt::TempFile::remove(std::string& filename)
{
    if(!(filename.empty()))
    {
        std::remove(filename.c_str());
        {
            std::lock_guard<std::mutex>(handler.tempfile_lock);
            handler.filenames.erase(filename);
        }
        filename.clear();
    }
}

//------------------------------------------------------------------------------

#include <unistd.h>
void
vcfbwt::truncate_file(std::string file_name, std::size_t new_size_in_bytes)
{
    int res = truncate(file_name.c_str(), new_size_in_bytes);
    if (res != 0) { spdlog::error("Error while truncating {} to {} bytes", file_name, new_size_in_bytes); }
}

bool
vcfbwt::is_gzipped(std::ifstream& in)
{
    in.seekg(0);
    char f, s; in.get(f); in.get(s);
    in.seekg(0);
    
    return ((f == '\x1F') and (s == '\x8B'));
}

//------------------------------------------------------------------------------

struct WritesCounter
{
    std::mutex write_stats_lock;
    std::size_t bytes_wrote;
    
    WritesCounter() : bytes_wrote(0) {};
    ~WritesCounter()
    {
        // Can't use spdlog here, could be destroyed after spdlog's objects
        std::cout << "[Disk Write (bytes): " << std::to_string(bytes_wrote) << "]" << std::endl;
    }
    
} writes_counter;

void vcfbwt::DiskWrites::update(std::size_t num_of_bytes)
{
    std::lock_guard<std::mutex> lock(writes_counter.write_stats_lock);
    writes_counter.bytes_wrote += num_of_bytes;
}



//------------------------------------------------------------------------------

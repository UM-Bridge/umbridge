#ifndef HPCIO_HPP
#define HPCIO_HPP

#include <iostream>
#include <fstream>
#include <vector>

class FileWriter {
public:
    FileWriter(const std::string& filepath) : filepath_(filepath) {
        file_.open(filepath_);
        if (!file_.is_open()) {
            std::cerr << "Failed to open file for writing: " << filepath_ << std::endl;
        }
    }

    void writeString(const std::string& value) {
        if (file_.is_open()) {
            file_ << value << std::endl;
        } else {
            std::cerr << "File is not open for writing: " << filepath_ << std::endl;
        }
    }

    void writeSizeT(std::size_t value) {
        if (file_.is_open()) {
            file_ << value << std::endl;
        } else {
            std::cerr << "File is not open for writing: " << filepath_ << std::endl;
        }
    }

    void writeInt(int value) {
        if (file_.is_open()) {
            file_ << value << std::endl;
        } else {
            std::cerr << "File is not open for writing: " << filepath_ << std::endl;
        }
    }

    void writeDouble(double value) {
        if (file_.is_open()) {
            file_ << value << std::endl;
        } else {
            std::cerr << "File is not open for writing: " << filepath_ << std::endl;
        }
    }

    void writeVectorString(const std::vector<std::string>& values) {
        if (file_.is_open()) {
            writeInt(values.size());
            for (const auto& value : values) {
                writeString(value);
            }
            file_ << std::endl;
        } else {
            std::cerr << "File is not open for writing: " << filepath_ << std::endl;
        }
    }

    void writeVectorSizeT(const std::vector<std::size_t>& values) {
        if (file_.is_open()) {
            writeInt(values.size());
            for (const auto& value : values) {
                writeSizeT(value);
            }
            file_ << std::endl;
        } else {
            std::cerr << "File is not open for writing: " << filepath_ << std::endl;
        }
    }

    void writeVectorDouble(const std::vector<double>& values) {
        if (file_.is_open()) {
            writeInt(values.size());
            for (const auto& value : values) {
                writeDouble(value);
            }
            file_ << std::endl;
        } else {
            std::cerr << "File is not open for writing: " << filepath_ << std::endl;
        }
    }

    void writeVectorVectorDouble(const std::vector<std::vector<double>>& values) {
        if (file_.is_open()) {
            writeInt(values.size());
            for (const auto& value : values) {
                writeVectorDouble(value);
            }
        } else {
            std::cerr << "File is not open for writing: " << filepath_ << std::endl;
        }
    }

    void close() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    ~FileWriter() {
        close();
    }

private:
    std::string filepath_;
    std::ofstream file_;
};

class FileReader {
public:
    FileReader(const std::string& filepath) : filepath_(filepath) {
        file_.open(filepath_);
        if (!file_.is_open()) {
            std::cerr << "Failed to open file for reading: " << filepath_ << std::endl;
        }
    }

    std::string readString() {
        std::string value;
        if (file_.is_open()) {
            file_ >> value;
        } else {
            std::cerr << "File is not open for reading: " << filepath_ << std::endl;
        }
        return value;
    }

    std::size_t readSizeT() {
        std::size_t value = 0;
        if (file_.is_open()) {
            file_ >> value;
        } else {
            std::cerr << "File is not open for reading: " << filepath_ << std::endl;
        }
        return value;
    }

    int readInt() {
        int value = 0;
        if (file_.is_open()) {
            file_ >> value;
        } else {
            std::cerr << "File is not open for reading: " << filepath_ << std::endl;
        }
        return value;
    }

    double readDouble() {
        double value = 0;
        if (file_.is_open()) {
            file_ >> value;
        } else {
            std::cerr << "File is not open for reading: " << filepath_ << std::endl;
        }
        return value;
    }

    std::vector<std::string> readVectorString() {
        std::vector<std::string> values;
        if (file_.is_open()) {
            int size = readInt();
            for (int i = 0; i < size; ++i) {
                values.push_back(readString());
            }
        } else {
            std::cerr << "File is not open for reading: " << filepath_ << std::endl;
        }
        return values;
    }

    std::vector<std::size_t> readVectorSizeT() {
        std::vector<std::size_t> values;
        if (file_.is_open()) {
            int size = readInt();
            for (int i = 0; i < size; ++i) {
                values.push_back(readSizeT());
            }
        } else {
            std::cerr << "File is not open for reading: " << filepath_ << std::endl;
        }
        return values;
    }

    std::vector<double> readVectorDouble() {
        std::vector<double> values;
        if (file_.is_open()) {
            int size = readInt();
            for (int i = 0; i < size; ++i) {
                values.push_back(readDouble());
            }
        } else {
            std::cerr << "File is not open for reading: " << filepath_ << std::endl;
        }
        return values;
    }

    std::vector<std::vector<double>> readVectorVectorDouble() {
        std::vector<std::vector<double>> values;
        if (file_.is_open()) {
            int size = readInt();
            for (int i = 0; i < size; ++i) {
                values.push_back(readVectorDouble());
            }
        } else {
            std::cerr << "File is not open for reading: " << filepath_ << std::endl;
        }
        return values;
    }

    void close() {
        if (file_.is_open()) {
            file_.close();
        }
    }

    ~FileReader() {
        close();
    }

private:
    std::string filepath_;
    std::ifstream file_;
};

#endif // HPCIO_HPP
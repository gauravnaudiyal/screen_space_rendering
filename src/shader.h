#pragma once
#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm.hpp>
#include <gtc/type_ptr.hpp>

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertPath, const char* fragPath) {
        std::string vertCode, fragCode;
        std::ifstream vFile, fFile;
        vFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            vFile.open(vertPath);
            fFile.open(fragPath);
            std::stringstream vStream, fStream;
            vStream << vFile.rdbuf();
            fStream << fFile.rdbuf();
            vertCode = vStream.str();
            fragCode = fStream.str();

            auto stripBOM = [](std::string& s) {
                if (s.size() >= 3 &&
                    (unsigned char)s[0] == 0xEF &&
                    (unsigned char)s[1] == 0xBB &&
                    (unsigned char)s[2] == 0xBF) {
                    s.erase(0, 3);
                }
                };
            stripBOM(vertCode);
            stripBOM(fragCode);

        }
        catch (std::ifstream::failure& e) {
            std::cerr << "ERROR::SHADER::FILE_NOT_READ: " << e.what() << std::endl;
        }
        const char* vCode = vertCode.c_str();
        const char* fCode = fragCode.c_str();

        unsigned int vert, frag;
        int success; char infoLog[512];

        vert = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 1, &vCode, NULL);
        glCompileShader(vert);
        glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vert, 512, NULL, infoLog);
            std::cerr << "ERROR::VERTEX::COMPILATION\n" << infoLog << std::endl;
        }

        frag = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 1, &fCode, NULL);
        glCompileShader(frag);
        glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(frag, 512, NULL, infoLog);
            std::cerr << "ERROR::FRAGMENT::COMPILATION\n" << infoLog << std::endl;
        }

        ID = glCreateProgram();
        glAttachShader(ID, vert);
        glAttachShader(ID, frag);
        glLinkProgram(ID);
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(ID, 512, NULL, infoLog);
            std::cerr << "ERROR::PROGRAM::LINKING\n" << infoLog << std::endl;
        }
        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    void use() { glUseProgram(ID); }
    void setBool(const std::string& n, bool v) { glUniform1i(glGetUniformLocation(ID, n.c_str()), (int)v); }
    void setInt(const std::string& n, int v) { glUniform1i(glGetUniformLocation(ID, n.c_str()), v); }
    void setFloat(const std::string& n, float v) { glUniform1f(glGetUniformLocation(ID, n.c_str()), v); }
    void setMat4(const std::string& n, const glm::mat4& m) {
        glUniformMatrix4fv(glGetUniformLocation(ID, n.c_str()), 1, GL_FALSE, glm::value_ptr(m));
    }
    void setVec3(const std::string& n, const glm::vec3& v) {
        glUniform3fv(glGetUniformLocation(ID, n.c_str()), 1, glm::value_ptr(v));
    }
};
#include <GL/glew.h>
#include <GL/glut.h>
#include <string>

#include "BufferObject.h"

void loadSource(GLuint &shaderID, std::string name);
void printCompileInfoLog(GLuint shadID);
void printLinkInfoLog(GLuint programID);
void validateProgram(GLuint programID);

class ShaderProgram {
	static const char* SHADER_FOLDER;

	private:
		GLuint programID;
		GLuint dispatch[3];

	public:
		ShaderProgram();
		~ShaderProgram();

		void addShader(GLuint type, std::string path);
		void compile();

		void uniformMatrix4fv(std::string str, GLuint n, GLboolean b, const GLfloat* pointer);

		void vertexAttribPointer(BufferObject<GL_SHADER_STORAGE_BUFFER>*, std::string str, GLuint size, GLuint type, GLboolean b, GLuint n, void* pointer);

		void setDispatch(GLuint x, GLuint y, GLuint z) {
			dispatch[0] = x;
			dispatch[1] = y;
			dispatch[2] = z;
		}

		static void use(ShaderProgram* sp) {
			glUseProgram(sp->programID);
		}

		static void useAtomic(ShaderProgram* sp) {
			glUseProgram(sp->programID);
			
			glDispatchCompute( sp->dispatch[0], sp->dispatch[1], sp->dispatch[2] );

			glMemoryBarrier( GL_SHADER_STORAGE_BARRIER_BIT );
		}

		GLuint id() {
			return programID;
		}
};
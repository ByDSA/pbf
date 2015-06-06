#include <GL/glew.h>
#include <GL/glut.h>
#include <string>

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
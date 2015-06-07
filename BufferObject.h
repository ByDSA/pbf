#ifndef _BUFFER_OBJECT_H
#define _BUFFER_OBJECT_H

#include <vector>

#include <glm/glm.hpp>

#include <GL/glew.h>
#include <GL/glut.h>

template<GLuint T>
class BufferObject {
	private:
		static GLuint BINDING_COUNTER;
		GLuint _id;
		GLuint binding_index;
	public:
		static const GLuint TYPE = T;

		BufferObject();

		template <typename U> void transfer(const std::vector<U>& v, GLuint mode);
		void allocate(GLuint size, GLuint mode);

		GLuint id() {
			return _id;
		}

};

template <GLuint T> GLuint BufferObject<T>::BINDING_COUNTER = 0;

template <GLuint T>
BufferObject<T>::BufferObject() {
	glGenBuffers(1, &_id);

	binding_index = BINDING_COUNTER;
	BINDING_COUNTER++;
}

template <GLuint T>
template <typename U>
void BufferObject<T>::transfer(const std::vector<U>& v, GLuint mode) {
	glBindBuffer(T, _id);
	glBufferData(T, sizeof(U)*v.size(), v.data(), mode);
	glBindBufferBase( T, binding_index, _id );
}

template <GLuint T>
void BufferObject<T>::allocate(GLuint size, GLuint mode) {
	glBindBuffer(T, _id);
	glBufferData(T, size, NULL, mode);
	glBindBufferBase( T, binding_index, _id );
}
#endif
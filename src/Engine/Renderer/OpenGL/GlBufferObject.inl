#include "GlBufferObject.hpp"
namespace Ra { namespace Engine
{

template<typename T, GLenum GL_BUFFER_TYPE>
inline GlBufferObject<T, GL_BUFFER_TYPE>::GlBufferObject()
    : m_numElements(0)
    , m_drawMode( GL_STREAM_DRAW )
{
    CORE_ASSERT(glGetString(GL_VERSION)!= 0, "GL context unavailable");
    GL_ASSERT( glGenBuffers(1, &m_bufferGlId));
}

template<typename T, GLenum GL_BUFFER_TYPE>
inline GlBufferObject<T, GL_BUFFER_TYPE>::GlBufferObject(const GlBufferObject<T,GL_BUFFER_TYPE>& buffer)
    : m_numElements( buffer.m_numElements )
    , m_drawMode( buffer.m_drawMode )
{
    CORE_ASSERT(glGetString(GL_VERSION)!= 0, "GL context unavailable");

    // create vbo_destination to hold part of vbo_src data
    GL_ASSERT( glGenBuffers(1, &m_bufferGlId) );
    bind();
    setData( m_numElements, 0, m_drawMode);

    // rebind buffers to copy from vbo_src to vbo_destination
    //buffer.bind();
    GL_ASSERT( glBindBuffer(GL_COPY_READ_BUFFER, buffer.getId()) );
    GL_ASSERT( glBindBuffer(GL_COPY_WRITE_BUFFER, this->getId()) );
    GL_ASSERT( glCopyBufferSubData(/*buffer.m_targetType*/GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, sizeof(T) * m_numElements) );
}

template<typename T, GLenum GL_BUFFER_TYPE>
inline GlBufferObject<T, GL_BUFFER_TYPE>::GlBufferObject(uint numElements, GLenum drawMode)
    : m_numElements(numElements)
    , m_drawMode( drawMode )
{
    CORE_ASSERT(glGetString(GL_VERSION)!= 0, "GL context unavailable");

    GL_ASSERT( glGenBuffers(1, &m_bufferGlId) );
    GL_ASSERT( glBindBuffer(GL_BUFFER_TYPE, m_bufferGlId) );
    GL_ASSERT( glBufferData(GL_BUFFER_TYPE, m_numElements* sizeof(T), 0, m_drawMode) );
    GL_ASSERT( glBindBuffer(GL_BUFFER_TYPE, 0) );
}

template<typename T, GLenum GL_BUFFER_TYPE>
inline GlBufferObject<T, GL_BUFFER_TYPE>::~GlBufferObject()
{
    if( getId() == getCurrentBinding() )
    {
        unbind();
    }
    GL_ASSERT( glDeleteBuffers(1, &m_bufferGlId) );
}

template<typename T, GLenum GL_BUFFER_TYPE>
inline void GlBufferObject<T, GL_BUFFER_TYPE>::bind() const
{
    GL_ASSERT( glBindBuffer(GL_BUFFER_TYPE, m_bufferGlId) );
}

template<typename T, GLenum GL_BUFFER_TYPE>
inline void GlBufferObject<T, GL_BUFFER_TYPE>::unbind() const
{
    GL_ASSERT( glBindBuffer(GL_BUFFER_TYPE, 0) );
}

template<typename T, GLenum GL_BUFFER_TYPE>
inline void GlBufferObject<T, GL_BUFFER_TYPE>::setData(uint numElements,
                               const T* data,
                               GLenum drawMode)
{
    bind();
    m_numElements = numElements;
    m_drawMode = drawMode;
    GL_ASSERT( glBufferData(GL_BUFFER_TYPE, m_numElements * sizeof(T), (GLvoid*)data, m_drawMode) );
    unbind();
}

template<typename T, GLenum GL_BUFFER_TYPE>
inline void GlBufferObject<T, GL_BUFFER_TYPE>::setData(const std::vector<T>& data, GLenum drawMode)
{
    setData(data.size(), &data.front(), drawMode);
}

template<typename T, GLenum GL_BUFFER_TYPE>
inline void GlBufferObject<T, GL_BUFFER_TYPE>::getData(uint numElements,
                                T* data, uint offset) const
{
    bind();
    GL_ASSERT( glGetBufferSubData(GL_BUFFER_TYPE, offset, numElements* sizeof(T), (GLvoid*)data) );
    unbind();
}

template<typename T, GLenum GL_BUFFER_TYPE>
inline T* GlBufferObject<T, GL_BUFFER_TYPE>::map(GLenum accessMode) const
{
    bind();
    CORE_ASSERT(m_numElements > 0, "No data to map");
    GL_ASSERT(T* data = (T*)glMapBuffer(GL_BUFFER_TYPE, accessMode));
    return data;
}

template<typename T, GLenum GL_BUFFER_TYPE>
inline bool GlBufferObject<T, GL_BUFFER_TYPE>::unmap() const
{
    bind();
    GL_ASSERT(bool state = glUnmapBuffer(GL_BUFFER_TYPE) ? true : false);
    return state;
}

}}

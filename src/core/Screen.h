//
// Created by George Huan on 2022/10/2.
//

#ifndef SCREEN_H
#define SCREEN_H

const float ScreenVertices[] = {
        -1.0f, 1.0f,   0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
        1.0f, -1.0f,   1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
        1.0f, -1.0f,   1.0f, 0.0f,
        1.0f,  1.0f,   1.0f, 1.0f
};

class Screen {
public:
    void InitScreenBind() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(ScreenVertices), ScreenVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
    }
    void DrawScreen() {
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    void Delete() {
        glDeleteBuffers(1, &VBO);
        glDeleteVertexArrays(1, &VAO);
    }
private:
    unsigned int VBO, VAO;
};

class ScreenFBO {
public:
    ScreenFBO(){ }
    void configuration(int SCR_WIDTH, int SCR_HEIGHT) {
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

        glGenTextures(1, &textureColorbuffer);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {

        }

        unBind();
    }

    void Bind() {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glDisable(GL_DEPTH_TEST);
    }

    void unBind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void BindAsTexture() {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    }

    void Delete() {
        unBind();
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteTextures(1, &textureColorbuffer);
    }

    unsigned int GetTextureColorBufferId() const {
        return framebuffer;
    }

    void Resize(int SCR_WIDTH, int SCR_HEIGHT) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, nullptr);
        unBind();
    }

private:
    unsigned int framebuffer;
    unsigned int textureColorbuffer;
    unsigned int rbo;
};


class RenderBuffer {
public:
    void Init(int SCR_WIDTH, int SCR_HEIGHT) {
        fbo[0].configuration(SCR_WIDTH, SCR_HEIGHT);
        fbo[1].configuration(SCR_WIDTH, SCR_HEIGHT);
        currentIndex = 0;
    }

    void Resize(int SCR_WIDTH, int SCR_HEIGHT) {
        fbo[0].Resize(SCR_WIDTH, SCR_HEIGHT);
        fbo[1].Resize(SCR_WIDTH, SCR_HEIGHT);
    }

    void setCurrentBuffer(int LoopNum) {
        int histIndex = LoopNum % 2;
        int curIndex = (histIndex == 0 ? 1 : 0);

        fbo[curIndex].Bind();
        fbo[histIndex].BindAsTexture();
    }
    void setCurrentAsTexture(int LoopNum) {
        int histIndex = LoopNum % 2;
        int curIndex = (histIndex == 0 ? 1 : 0);
        fbo[curIndex].BindAsTexture();
    }

    unsigned int getCurrentTexture(int LoopNum) {
        int histIndex = LoopNum % 2;
        int curIndex = (histIndex == 0 ? 1 : 0);
        return fbo[curIndex].GetTextureColorBufferId();
    }

    void Delete() {
        fbo[0].Delete();
        fbo[1].Delete();
    }
private:
    int currentIndex;
    ScreenFBO fbo[2];
};



#endif //SCREEN_H

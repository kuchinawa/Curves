/**********************************************************************************
// Curves (Código Fonte)
//
// Criação:     12 Ago 2020
// Atualização: 12 Ago 2024
// Compilador:  Visual C++ 2022
//
// Descrição:   Base para gerar curvas usando Corner-Cutting
//
**********************************************************************************/

#include "DXUT.h"

// ------------------------------------------------------------------------------

struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

// ------------------------------------------------------------------------------

class Curves : public App
{
private:
    ID3D12RootSignature* rootSignature;
    ID3D12PipelineState* pipelineState;
    Mesh* geometry;
    Mesh* curve;
    Mesh* control;

    static const uint MaxVertex = 21 * 4;
    static const uint MaxCurve = 30;
    Vertex vertices[MaxVertex + 1];
    Vertex linhas[4];
    Vertex curves[MaxCurve * (MaxVertex / 4)];
    Vertex controle[7 * MaxVertex];

    uint count = 0;
    uint index = 0;
    uint controlCount = 0;

    bool finalizado = false;

    // Buffers para salvar e carregar
    Vertex savedVertices[MaxVertex + 2];
    Vertex savedCurves[MaxCurve * (MaxVertex / 4)];
    Vertex savedLinhas[4];
    Vertex savedControle[7 * MaxVertex];
    uint savedCount = 0;
    uint savedIndex = 0;


public:
    void Init();
    void Update();
    void Display();
    void Finalize();

    void BuildRootSignature();
    void BuildPipelineState();

    void Bezier(XMFLOAT3 p0, XMFLOAT3 p1, XMFLOAT3 p2, XMFLOAT3 p3, float t, XMFLOAT3& p);
};

// ------------------------------------------------------------------------------

void Curves::Init()
{
    graphics->ResetCommands();

    // ---------[ Build Geometry ]------------

    // tamanho do buffer de vértices em bytes
    const uint vbSize = MaxVertex * sizeof(Vertex);

    // cria malha 3D
    geometry = new Mesh(4 * sizeof(Vertex), sizeof(Vertex));
    //geometry = new Mesh(vbSize, sizeof(Vertex));
    curve = new Mesh(MaxCurve * (MaxVertex / 4) * sizeof(Vertex), sizeof(Vertex));
    control = new Mesh(MaxVertex * 7 * sizeof(Vertex), sizeof(Vertex));

    // ---------------------------------------

    BuildRootSignature();
    BuildPipelineState();

    // ---------------------------------------

    graphics->SubmitCommands();
}

// ------------------------------------------------------------------------------

void Curves::Update()
{
    // sai com o pressionamento da tecla ESC
    if (input->KeyPress(VK_ESCAPE))
        window->Close();


    float cx = float(window->CenterX());
    float cy = float(window->CenterY());
    float mx = float(input->MouseX());
    float my = float(input->MouseY());

    // converte as coordenadas da tela para a faixa -1.0 a 1.0
    // cy e my foram invertidos para levar em consideração que 
    // o eixo y da tela cresce na direção oposta do cartesiano
    float x = (mx - cx) / cx;
    float y = (cy - my) / cy;
    if (index < MaxVertex){
        vertices[index] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::Gray) };
        linhas[index % 4] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::Gray) };

        controle[index * 7 + 0] = { XMFLOAT3(x, y - 0.025f, 0.0f), XMFLOAT4(Colors::LightPink) };       // Vértice inferior (antes era superior)
        controle[index * 7 + 1] = { XMFLOAT3(x - 0.025f, y, 0.0f), XMFLOAT4(Colors::LightPink) };        // Esquerda superior (sem mudança)
        controle[index * 7 + 2] = { XMFLOAT3(x - 0.015f, y + 0.025f, 0.0f), XMFLOAT4(Colors::LightPink) }; // Esquerda inferior (invertido)
        controle[index * 7 + 3] = { XMFLOAT3(x, y + 0.015f, 0.0f), XMFLOAT4(Colors::LightPink) };        // Centro superior (invertido)
        controle[index * 7 + 4] = { XMFLOAT3(x + 0.015f, y + 0.025f, 0.0f), XMFLOAT4(Colors::LightPink) }; // Direita inferior (invertido)
        controle[index * 7 + 5] = { XMFLOAT3(x + 0.025f, y, 0.0f), XMFLOAT4(Colors::LightPink) };        // Direita superior (sem mudança)
        controle[index * 7 + 6] = { XMFLOAT3(x, y - 0.025f, 0.0f), XMFLOAT4(Colors::LightPink) };       // Volta ao vértice inferior (antes era superior)
        
        if (count % 4 == 3)
        {
            uint i = count - 3;
            XMFLOAT3 p0 = vertices[i].Pos;
            XMFLOAT3 p1 = vertices[i + 1].Pos;
            XMFLOAT3 p2 = vertices[i + 2].Pos;
            XMFLOAT3 p3 = { x, y, 0.0f };

            for (uint j = 0; j < MaxCurve; ++j)
            {
                float t = float(j) / float(MaxCurve - 1);
                XMFLOAT3 p{};
                Bezier(p0, p1, p2, p3, t, p);
                curves[((i / 4) * MaxCurve) + j] = { p, XMFLOAT4(Colors::HotPink) };

            }
        }
    }
    else
    {
		finalizado = true;
	}


    // cria vértices com o botão do mouse
    if (input->KeyPress(VK_LBUTTON))
    {

        if (index < MaxVertex)
            index++;

        if (count < MaxVertex)
            ++count;

        controlCount++;

        if (count % 4 == 0 && count > 0) {
            for (uint i = 0; i < count - 3; i += 4)
            {
                XMFLOAT3 p0 = vertices[i].Pos;
                XMFLOAT3 p1 = vertices[i + 1].Pos;
                XMFLOAT3 p2 = vertices[i + 2].Pos;
                XMFLOAT3 p3 = vertices[i + 3].Pos;

                for (uint j = 0; j < MaxCurve; ++j)
                {
                    float t = float(j) / float(MaxCurve - 1);
                    XMFLOAT3 p{};
                    Bezier(p0, p1, p2, p3, t, p);
                    curves[((i / 4) * MaxCurve) + j] = { p, XMFLOAT4(Colors::HotPink) };
                }
            }


            if (count < MaxVertex) {

                OutputDebugString("Entrei");

                XMFLOAT3 lastp = vertices[(index - 1) % MaxVertex].Pos;
                XMFLOAT3 lastp1 = vertices[(index - 2) % MaxVertex].Pos;

                vertices[index] = { XMFLOAT3(x,y, 0.0f), XMFLOAT4(Colors::Gray) };
                linhas[index % 4] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::Gray) };

                if (index < MaxVertex - 1)
                    index++;

                if (count < MaxVertex - 1)
                    ++count;

                vertices[index] = {
                    XMFLOAT3(
                        lastp.x + (lastp.x - lastp1.x),
                        lastp.y + (lastp.y - lastp1.y),

                        0.0f),
                    XMFLOAT4(Colors::Gray)
                };
                linhas[index % 4] = { XMFLOAT3(lastp.x + (lastp.x - lastp1.x), lastp.y + (lastp.y - lastp1.y), 0.0f), XMFLOAT4(Colors::Gray) };

                OutputDebugString(std::to_string(index).c_str());
                // atualiza vertices para triângulo de controle no mouse
                controle[index * 7 + 0] = { XMFLOAT3(vertices[index].Pos.x, vertices[index].Pos.y - 0.025f, 0.0f), XMFLOAT4(Colors::LightPink) };       // Vértice inferior (antes era superior)
                controle[index * 7 + 1] = { XMFLOAT3(vertices[index].Pos.x - 0.025f, vertices[index].Pos.y, 0.0f), XMFLOAT4(Colors::LightPink) };        // Esquerda superior (sem mudança)
                controle[index * 7 + 2] = { XMFLOAT3(vertices[index].Pos.x - 0.015f, vertices[index].Pos.y + 0.025f, 0.0f), XMFLOAT4(Colors::LightPink) }; // Esquerda inferior (invertido)
                controle[index * 7 + 3] = { XMFLOAT3(vertices[index].Pos.x, vertices[index].Pos.y + 0.015f, 0.0f), XMFLOAT4(Colors::LightPink) };        // Centro superior (invertido)
                controle[index * 7 + 4] = { XMFLOAT3(vertices[index].Pos.x + 0.015f, vertices[index].Pos.y + 0.025f, 0.0f), XMFLOAT4(Colors::LightPink) }; // Direita inferior (invertido)
                controle[index * 7 + 5] = { XMFLOAT3(vertices[index].Pos.x + 0.025f, vertices[index].Pos.y, 0.0f), XMFLOAT4(Colors::LightPink) };        // Direita superior (sem mudança)
                controle[index * 7 + 6] = { XMFLOAT3( vertices[index].Pos.x, vertices[index].Pos.y - 0.025f, 0.0f), XMFLOAT4(Colors::LightPink) };       // Volta ao vértice inferior (antes era superior)


                if (index < MaxVertex - 1)
                    index++;

                if (count < MaxVertex - 1)
                    ++count;
            }
        }

    }

    // Salvar os buffers em memória ao pressionar 'S'
    if (input->KeyPress('S'))
    {
        for (uint i = 0; i < MaxVertex + 1; ++i)
        {
            savedVertices[i] = vertices[i];
        }
        for (uint i = 0; i < MaxCurve * (MaxVertex / 4); ++i)
        {
            savedCurves[i] = curves[i];
        }
        for (uint i = 0; i < 7 * MaxVertex; ++i)
        {
            savedControle[i] = controle[i];
        }
        for (uint i = 0; i < 4; ++i)
        {
            savedLinhas[i] = linhas[i];
        }
        savedCount = count;
        savedIndex = index;
    }

    // Carregar os buffers da memória ao pressionar 'L'
    if (input->KeyPress('L'))
    {
        for (uint i = 0; i < MaxVertex + 1; ++i)
        {
            vertices[i] = savedVertices[i];
        }
        for (uint i = 0; i < MaxCurve * (MaxVertex / 4); ++i)
        {
            curves[i] = savedCurves[i];
        }
        for (uint i = 0; i < 7 * MaxVertex; ++i)
        {
            controle[i] = savedControle[i];
        }
        for (uint i = 0; i < 4; ++i)
        {
            linhas[i] = savedLinhas[i];
        }
        count = savedCount;
        index = savedIndex;
    }

    // Apagar as curvas e linhas ao pressionar 'D' ou 'DELETE'
    if (input->KeyPress('D') || input->KeyPress(VK_DELETE))
    {
        for (uint i = 0; i < MaxVertex; ++i)
        {
            vertices[i] = {};
        }
        for (uint i = 0; i < MaxCurve * (MaxVertex / 4); ++i)
        {
            curves[i] = {};
        }
        for (uint i = 0; i < 7 * MaxVertex; ++i)
        {
            controle[i] = {};
        }
        count = 0;
        index = 0;
    }


    // copia vértices para o buffer da GPU usando o buffer de Upload
    graphics->ResetCommands();
    graphics->Copy(curves, curve->vertexBufferSize, curve->vertexBufferUpload, curve->vertexBufferGPU);
    //graphics->Copy(vertices, geometry->vertexBufferSize, geometry->vertexBufferUpload, geometry->vertexBufferGPU);
    graphics->Copy(linhas, geometry->vertexBufferSize, geometry->vertexBufferUpload, geometry->vertexBufferGPU);
    graphics->Copy(controle, control->vertexBufferSize, control->vertexBufferUpload, control->vertexBufferGPU);
    graphics->SubmitCommands();
    //if (index < MaxVertex)
    Display();
}

void Curves::Bezier(XMFLOAT3 p0, XMFLOAT3 p1, XMFLOAT3 p2, XMFLOAT3 p3, float t, XMFLOAT3& p)
{
    float u = 1 - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    p.x = uuu * p0.x;
    p.x += 3 * uu * t * p1.x;
    p.x += 3 * u * tt * p2.x;
    p.x += ttt * p3.x;

    p.y = uuu * p0.y;
    p.y += 3 * uu * t * p1.y;
    p.y += 3 * u * tt * p2.y;
    p.y += ttt * p3.y;

}

// ------------------------------------------------------------------------------

void Curves::Display()
{
    // limpa backbuffer
    graphics->Clear(pipelineState);

    // submete comandos de configuração do pipeline
    graphics->CommandList()->SetGraphicsRootSignature(rootSignature);
    graphics->CommandList()->IASetVertexBuffers(0, 1, geometry->VertexBufferView());
    graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

    // submete comandos de desenho
    if (!finalizado)
        graphics->CommandList()->DrawInstanced((index % 4) + 1, 1, 0, 0);
	else
		graphics->CommandList()->DrawInstanced((index % 4), 1, 0, 0);

    graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);
    graphics->CommandList()->IASetVertexBuffers(0, 1, curve->VertexBufferView());

    if (count % 4 == 3)
        graphics->CommandList()->DrawInstanced(MaxCurve * ((count + 3) / 4), 1, 0, 0);
    else
        if (!finalizado)
            graphics->CommandList()->DrawInstanced(MaxCurve * ((count) / 4), 1, 0, 0);
        else
            graphics->CommandList()->DrawInstanced(MaxCurve * ((count) / 4), 1, 0, 0);


    graphics->CommandList()->IASetVertexBuffers(0, 1, control->VertexBufferView());


    for (uint i = max(0, index - 2); i < index + 1; ++i)
	{
        graphics->CommandList()->DrawInstanced(7, 1, i * 7, 0);
	}


    // apresenta backbuffer
    graphics->Present();
}

// ------------------------------------------------------------------------------

void Curves::Finalize()
{
    rootSignature->Release();
    pipelineState->Release();
    delete geometry;
    delete curve;
    delete control;
}


// ------------------------------------------------------------------------------
//                                     D3D                                      
// ------------------------------------------------------------------------------

void Curves::BuildRootSignature()
{
    // descrição para uma assinatura vazia
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 0;
    rootSigDesc.pParameters = nullptr;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // serializa assinatura raiz
    ID3DBlob* serializedRootSig = nullptr;
    ID3DBlob* error = nullptr;

    ThrowIfFailed(D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &error));

    // cria uma assinatura raiz vazia
    ThrowIfFailed(graphics->Device()->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)));
}

// ------------------------------------------------------------------------------

void Curves::BuildPipelineState()
{
    // --------------------
    // --- Input Layout ---
    // --------------------

    D3D12_INPUT_ELEMENT_DESC inputLayout[2] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // --------------------
    // ----- Shaders ------
    // --------------------

    ID3DBlob* vertexShader;
    ID3DBlob* pixelShader;

    D3DReadFileToBlob(L"Shaders/Vertex.cso", &vertexShader);
    D3DReadFileToBlob(L"Shaders/Pixel.cso", &pixelShader);

    // --------------------
    // ---- Rasterizer ----
    // --------------------

    D3D12_RASTERIZER_DESC rasterizer = {};
    rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.FrontCounterClockwise = FALSE;
    rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = FALSE;
    rasterizer.AntialiasedLineEnable = FALSE;
    rasterizer.ForcedSampleCount = 0;
    rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // ---------------------
    // --- Color Blender ---
    // ---------------------

    D3D12_BLEND_DESC blender = {};
    blender.AlphaToCoverageEnable = FALSE;
    blender.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
    {
        FALSE,FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        blender.RenderTarget[i] = defaultRenderTargetBlendDesc;

    // ---------------------
    // --- Depth Stencil ---
    // ---------------------

    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = TRUE;
    depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencil.StencilEnable = FALSE;
    depthStencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
    { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    depthStencil.FrontFace = defaultStencilOp;
    depthStencil.BackFace = defaultStencilOp;

    // -----------------------------------
    // --- Pipeline State Object (PSO) ---
    // -----------------------------------

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = rootSignature;
    pso.VS = { reinterpret_cast<BYTE*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    pso.PS = { reinterpret_cast<BYTE*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    pso.BlendState = blender;
    pso.SampleMask = UINT_MAX;
    pso.RasterizerState = rasterizer;
    pso.DepthStencilState = depthStencil;
    pso.InputLayout = { inputLayout, 2 };
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pso.SampleDesc.Count = graphics->Antialiasing();
    pso.SampleDesc.Quality = graphics->Quality();
    graphics->Device()->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineState));

    vertexShader->Release();
    pixelShader->Release();
}

// ------------------------------------------------------------------------------
//                                  WinMain                                      
// ------------------------------------------------------------------------------

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    try
    {
        // cria motor e configura a janela
        Engine* engine = new Engine();
        engine->window->Mode(WINDOWED);
        engine->window->Size(1024, 600);
        engine->window->ResizeMode(ASPECTRATIO);
        engine->window->Color(0, 0, 0);
        //engine->window->Color(0, 0, 0);
        engine->window->Title("Curves");
        engine->window->Icon(IDI_ICON);
        engine->window->LostFocus(Engine::Pause);
        engine->window->InFocus(Engine::Resume);

        // cria e executa a aplicação
        engine->Start(new Curves());

        // finaliza execução
        delete engine;
    }
    catch (Error& e)
    {
        // exibe mensagem em caso de erro
        MessageBox(nullptr, e.ToString().data(), "Curves", MB_OK);
    }

    return 0;
}

// ----------------------------------------------------------------------------



A ideie é usar a plyfile, mas tem que gera um vetor de caracteristica de um Subgraph usando a biblioteca pytohn `opfppy` e codificar as propriedades de splats gaussainos que são as propriedades que são listadas no cabeçalho .ply:
```ply
ply
format binary_little_endian 1.0                                                                              
element vertex 35721                                                                                         
property float x                                                                                             
property float y                                                                                             
property float z
property float nx
property float ny
property float nz
property float f_dc_0
property float f_dc_1
property float f_dc_2
property float f_rest_0
property float f_rest_1
property float f_rest_2
property float f_rest_3
property float f_rest_4
property float f_rest_5
property float f_rest_6
property float f_rest_7
property float f_rest_8
format binary_little_endian 1.0                                                                              
element vertex 35721                                                                                         
property float x                                                                                             
property float y                                                                                             
property float z
property float nx
property float ny
property float nz
property float f_dc_0
property float f_dc_1
property float f_dc_2
property float f_rest_0
property float f_rest_1
property float f_rest_2
property float f_rest_3
property float f_rest_4
property float f_rest_5
property float f_rest_6
property float f_rest_7
property float f_rest_8
property float f_rest_9
property float f_rest_10
property float f_rest_11
property float f_rest_12
property float f_rest_13
property float f_rest_14
property float f_rest_15
property float f_rest_16
property float f_rest_17
property float f_rest_18
property float f_rest_19
property float f_rest_20
property float f_rest_21
property float f_rest_22
property float f_rest_23
property float f_rest_24
property float f_rest_25
property float f_rest_26
property float f_rest_27
property float f_rest_28
property float f_rest_29 
property float f_rest_30
property float f_rest_31
property float f_rest_32
property float f_rest_33
property float f_rest_34
property float f_rest_35
property float f_rest_36
property float f_rest_37
property float f_rest_38
property float f_rest_39
property float f_rest_40
property float f_rest_41
property float f_rest_42
property float f_rest_43
property float f_rest_44
property float opacity
property float scale_0
property float scale_1
property float scale_2
property float rot_0
property float rot_1
property float rot_2
property float rot_3
end_header 
```

O restandte do arquivo é  escrito de forma binária seguinda a ordem das propriedades. O objetivo é fazer um script python que não somente faça a leitura usando a biblioteca [plyfile](https://pypi.org/project/plyfile), mas também crias atributos para o cada Node (que seria um splat) e o Subgraph (que seria a cena).

# Plano de implementação:

[ ] Compactação em memmória:  compacta os coeficientes hamônicas paraa cada coeficiente por seu número em formato binário, em até um byte, com possibilidade de armazenar um tuple de grau, indices concatenada em bits formando até 8 bit (3 bits para os graus, 5 bits para o indice do ceoficiente)

 Para criar esse mapeamento, vamos transformar a estrutura do seu arquivo .ply em um array NumPy onde cada entrada é um byte (8 bits) codificado conforme sua regra.

🧬 Estratégia de Codificação
Como você definiu:

3 bits para o Grau ($l$): Permite representar de 0 a 7 (2^3=8).

5 bits para o Índice ($m$): Permite representar de -16 a 15 (ou 0 a 31 se tratarmos como índice relativo).

No Gaussian Splatting, para um grau $l$, o índice m varia de $−l$ até $l$. Para caber em 5 bits de forma simples e segura, podemos usar o valor de $m$ deslocado ou o índice sequencial dentro do nível. Vamos usar a lógica de deslocamento de bits em Python: `(l << 5) | (m_encoded)`.

💻 Esboço do Programa
Para começarmos, veja como estruturaríamos a lógica de extração e empacotamento:

```Python
import numpy as np
from plyfile import PlyData

def encode_sh_params(l, m):
    # m_encoded: transformamos o m (que pode ser negativo) 
    # em um valor positivo para os 5 bits (ex: somando 16)
    m_shifted = m + 16 
    return (l << 5) | (m_shifted & 0x1F)

def process_ply_to_subgraph(ply_path, output_path):
    plydata = PlyData.read(ply_path)
    vertices = plydata['vertex']
    
    # Identificar as colunas f_dc e f_rest
    # f_dc_0, 1, 2 -> l=0, m=0 (1 coeficiente por canal)
    # f_rest_0...44 -> l=1,2,3... (15 coeficientes por canal)
    
    # Vamos focar em extrair a 'assinatura' de índices para os 16 coeficientes totais
    indices_map = []
    
    # Grau 0 (DC)
    indices_map.append(encode_sh_params(0, 0))
    
    # Graus 1 a 3 (Rest)
    # No seu PLY, f_rest_0, f_rest_1, f_rest_2 é o primeiro SH de alta ordem
    for i in range(15):
        # Usando a fórmula que discutimos antes:
        i_virtual = i + 1
        l = int(i_virtual**0.5)
        m = i_virtual - (l**2) - l
        indices_map.append(encode_sh_params(l, m))
    
    # Converter para array de bytes e salvar
    encoded_array = np.array(indices_map, dtype=np.uint8)
    np.save(output_path, encoded_array)
    return encoded_array

Exemplo de uso:
`process_ply_to_subgraph('point_cloud.ply', 'point_subgraph.sg')`

🔍 Vamos validar a lógica dos bits?
Antes de rodarmos o código completo no seu arquivo, vamos conferir se o "espaço" de 8 bits é suficiente para o que você precisa.

Se pegarmos o coeficiente de maior ordem no seu arquivo ($l=3$):

O grau l=3 em binário é 011.

O índice m pode chegar a +3. Se usarmos o deslocamento de +16 para evitar números negativos, m vira 19, que é 10011 em binário.

O byte final seria 011 + 10011 = 01110011.

Essa estrutura de 8 bits funciona perfeitamente para SH de até grau 7! 🚀

Para finalizar o script para você, como você prefere tratar a variável m dentro desses 5 bits?

Valor absoluto com sinal: Reservar 1 bit para o sinal (+/-) e 4 para o valor.

Offset (Nossa sugestão): Somar um valor fixo (ex: 16) para que todos os m fiquem entre 0 e 31.

Índice Linear: Apenas guardar a posição sequencial do coeficiente dentro daquele grau $l$.
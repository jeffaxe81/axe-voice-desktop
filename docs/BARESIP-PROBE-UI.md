# Verificação do Baresip na interface

Na interface do AXE Voice, clique em **Verificar Baresip local**. O aplicativo procura um arquivo chamado `baresip.exe` **na mesma pasta de `AXE-Voice.exe`**. Mostra se o arquivo está ausente ou presente, sem executá-lo.

Esse teste não confirma versão, autenticidade, módulos de áudio, conectividade SIP nem registro no PBX. A instalação deve ser obtida de fonte confiável e verificada antes de qualquer execução. A futura integração deverá usar um backend controlado, eventos de registro, chamadas e testes bidirecionais de áudio.

Para testar localmente após compilar:

```powershell
cd C:\dev\axe-voice-v016
git pull --ff-only
cmake --build build --config Release --parallel 4
ctest --test-dir build -C Release --output-on-failure
& '.\build\Release\AXE-Voice.exe'
```

## UnityAssemblyInjector
This library is used for loading custom c# assemblies into Unity games' runtime without having to modify any game files.


## How to use
Place the dll file next to the game's main directory, if it's a 32bit unity game, use version-x86.dll, otherwise, use version-x64.dll (rename to version.dll in either case)

Create an assemblies.txt which will contain the path to your c# dlls and a static entry point path, i.e. `SomeMod\\SomeLoader.dll=SomeNamespace.SomeClass:StaticInitMethod`. you can have as many of these as you want, one per line.

The basic structure of the c# project would be as such
```csharp
namespace SomeNamespace
{
    public class SomeClass
    {
        public static void StaticInitMethod()
        {
            new Thread(() =>
            {
                Thread.Sleep(5000); // 5 second sleep as initialization occurs *really* early

                GameObject someObject = new GameObject();
                someObject.AddComponent<SomeComponent>(); // MonoBehaviour
                UnityEngine.Object.DontDestroyOnLoad(someObject);

            }).Start();
        }
    }
}
```

## Downloads
Downloads can be found in the [releases](https://github.com/avail/UnityAssemblyInjector/releases) tab.
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.InteropServices;


public class SimplechBridging : MonoBehaviour {


	#if UNITY_IPHONE
	// On iOS plugins are statically linked into the executable, so we have to use __Internal as the library name.
	[DllImport ("__Internal")]
	#else
	// Other platforms load plugins dynamically, so pass the name of the plugin's dynamic library.
	[DllImport ("simplech")]
	#endif
	private static extern int [,] fillArray(int size);

	// Use this for initialization
	void Start () {
		Debug.Log ("[simplech] SimplechBridging behaviour starting");
		ArrayFillTest(); 
	}

	private void ArrayFillTest() { 
		int size = 512; 
		var start = Time.realtimeSinceStartup; 
		int[,] tab = fillArray(512); 
		Debug.Log("[simplech] test run: array of size " + size.ToString() + " filled in " + (Time.realtimeSinceStartup-start).ToString("f6") + " secs."); 
	} 
	
	// Update is called once per frame
	void Update () {
		
	}
}

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

public class ContactReceiver : MonoBehaviour
{
	public UnityEvent myEvent;

	void OnTriggerEnter(Collider otherer)
	{
		if (otherer.CompareTag("Cube"))
		{
			this.myEvent.Invoke();
		}
	}
}

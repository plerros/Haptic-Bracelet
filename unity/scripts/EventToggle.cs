using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

public class EventToggle : MonoBehaviour
{
	public bool m_runEvent;
	public UnityEvent m_Event;

	
	// Start is called before the first frame update
	void Start()
	{
	
	}

	// Update is called once per frame
	void Update()
	{
		if (! m_runEvent)
			return;
		m_Event.Invoke();
		m_runEvent = false;
	}
}

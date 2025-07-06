using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Events;

public class TestState : MonoBehaviour
{
	public GameObject[] m_GameObjects;
	public int          m_Position = 0;
	public UnityEvent   m_Enter0Event;
	public UnityEvent   m_Exit0Event;

	public void Start()
	{
		nextStateInternal();
		while (m_Position != 0)
			nextStateInternal();
	}

	public void nextState()
	{
		Invoke(nameof(deactivateCurrent), 0.1f);
		Invoke(nameof(activeNext), 0.1f);
	}

	private void nextStateInternal()
	{
		deactivateCurrent();
		activeNext();
	}

	private void deactivateCurrent()
	{
		if (m_Position == 0 && m_Exit0Event != null)
		{	
			m_Exit0Event.Invoke();
		}
		m_GameObjects[m_Position].SetActive(false);
		m_Position++;
		if (m_Position >= m_GameObjects.Length)
			m_Position = 0;
	}

	private void activeNext()
	{
		if (m_Position == 0 && m_Enter0Event != null)
		{	
			m_Enter0Event.Invoke();
		}
		m_GameObjects[m_Position].SetActive(true);
	}
}

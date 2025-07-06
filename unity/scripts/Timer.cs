using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Text;
using System.IO;
using System;
using UnityEngine.Events;

public class Timer : MonoBehaviour
{
	private System.DateTime m_StartTime;
	private Boolean         m_IsRunning  = false; // the timer is running
	private System.DateTime m_SelectionTime;
	private List<double>    m_Data       = new List<double>();
	private String          m_FilePath;
	private GameObject      m_UserIdGameObject;


	public int              m_Iterations;
	public String           m_DataLabel;
	public UnityEvent       m_Event;

	void Start()
	{
		m_FilePath = Application.dataPath + "/results/" +  m_DataLabel + ".csv";

		StreamWriter writer = new StreamWriter(m_FilePath, true, Encoding.ASCII);
		writer.Close();

		m_UserIdGameObject = GameObject.Find("ExperimentControls");
	}

	public void StartTimer()
	{
		m_StartTime = DateTime.Now;
		m_IsRunning = true;
		Debug.Log("Timer Started");
	}

	public void WasSelected()
	{
		if (m_IsRunning == false)
			return;

		m_SelectionTime = DateTime.Now;
		Debug.Log("Timer Selected");
	}

	public void StopTimer()
	{
		if (m_IsRunning == false)
			return;


		if (m_Data.Count < m_Iterations) {
			double selectionSeconds = (m_SelectionTime - m_StartTime).TotalSeconds;
			double positioningSeconds = (DateTime.Now - m_SelectionTime).TotalSeconds;

			m_Data.Add(positioningSeconds);
			StreamWriter writer = new StreamWriter(m_FilePath, true, Encoding.ASCII);
			writer.WriteLine((m_UserIdGameObject.GetComponent<UserID>()).userID + "," + selectionSeconds + "," + positioningSeconds);
			writer.Close();
		}


		if (m_Data.Count >= m_Iterations && m_Event != null)
			m_Event.Invoke();

		m_IsRunning = false;
		Debug.Log("Timer Stopped");
	}

	public void ResetData()
	{
		m_Data.Clear();
	}
}

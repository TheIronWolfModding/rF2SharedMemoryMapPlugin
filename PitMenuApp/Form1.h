#pragma once
#include "HWControl.h"

namespace CppCLRWinformsProjekt {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Zusammenfassung für Form1
	/// </summary>
	public ref class Form1 : public System::Windows::Forms::Form
	{
	public:
		Form1(void)
		{
			InitializeComponent();
			//
			//TODO: Konstruktorcode hier hinzufügen.
			//
		}

	protected:
		/// <summary>
		/// Verwendete Ressourcen bereinigen.
		/// </summary>
		~Form1()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::TableLayoutPanel^ tableLayoutPanel1;
	protected:
	private: System::Windows::Forms::TextBox^ textBox1;
	private: System::Windows::Forms::Label^ label1;

	private:
		/// <summary>
		/// Erforderliche Designervariable.
		/// </summary>
		System::ComponentModel::Container ^components;
    HWControl oHWControl;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Erforderliche Methode für die Designerunterstützung.
		/// Der Inhalt der Methode darf nicht mit dem Code-Editor geändert werden.
		/// </summary>
		void InitializeComponent(void)
		{
      this->tableLayoutPanel1 = (gcnew System::Windows::Forms::TableLayoutPanel());
      this->textBox1 = (gcnew System::Windows::Forms::TextBox());
      this->label1 = (gcnew System::Windows::Forms::Label());
      this->tableLayoutPanel1->SuspendLayout();
      this->SuspendLayout();
      // 
      // tableLayoutPanel1
      // 
      this->tableLayoutPanel1->ColumnCount = 2;
      this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
        46.02941F)));
      this->tableLayoutPanel1->ColumnStyles->Add((gcnew System::Windows::Forms::ColumnStyle(System::Windows::Forms::SizeType::Percent,
        53.97059F)));
      this->tableLayoutPanel1->Controls->Add(this->textBox1, 0, 0);
      this->tableLayoutPanel1->Controls->Add(this->label1, 1, 0);
      this->tableLayoutPanel1->Location = System::Drawing::Point(12, 59);
      this->tableLayoutPanel1->Name = L"tableLayoutPanel1";
      this->tableLayoutPanel1->RowCount = 2;
      this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 43.72093F)));
      this->tableLayoutPanel1->RowStyles->Add((gcnew System::Windows::Forms::RowStyle(System::Windows::Forms::SizeType::Percent, 56.27907F)));
      this->tableLayoutPanel1->Size = System::Drawing::Size(709, 430);
      this->tableLayoutPanel1->TabIndex = 0;
      this->tableLayoutPanel1->Paint += gcnew System::Windows::Forms::PaintEventHandler(this, &Form1::tableLayoutPanel1_Paint);
      // 
      // textBox1
      // 
      this->textBox1->Anchor = static_cast<System::Windows::Forms::AnchorStyles>((((System::Windows::Forms::AnchorStyles::Top | System::Windows::Forms::AnchorStyles::Bottom)
        | System::Windows::Forms::AnchorStyles::Left)
        | System::Windows::Forms::AnchorStyles::Right));
      this->textBox1->Font = (gcnew System::Drawing::Font(L"LCDMono", 15.85714F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
        static_cast<System::Byte>(0)));
      this->textBox1->Location = System::Drawing::Point(3, 3);
      this->textBox1->Multiline = true;
      this->textBox1->Name = L"textBox1";
      this->textBox1->ReadOnly = true;
      this->textBox1->Size = System::Drawing::Size(320, 181);
      this->textBox1->TabIndex = 0;
      this->textBox1->Text = L"FUEL: +10.0/4\r\nFL PRESS: 20.0\r\nFR PRESS: 20.0\r\nRL PRESS: 18.5\r\nRR PRESS: 18.5";
      this->textBox1->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &Form1::Form1_KeyDown);
      this->textBox1->KeyUp += gcnew System::Windows::Forms::KeyEventHandler(this, &Form1::Form1_KeyUp);
      // 
      // label1
      // 
      this->label1->AutoSize = true;
      this->label1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 12, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
        static_cast<System::Byte>(0)));
      this->label1->Location = System::Drawing::Point(336, 10);
      this->label1->Margin = System::Windows::Forms::Padding(10);
      this->label1->Name = L"label1";
      this->label1->Size = System::Drawing::Size(350, 64);
      this->label1->TabIndex = 1;
      this->label1->Text = L"Use ASDW as cursor keys \r\nto control the Pit Menu";
      // 
      // Form1
      // 
      this->AutoScaleDimensions = System::Drawing::SizeF(9, 20);
      this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
      this->ClientSize = System::Drawing::Size(733, 546);
      this->Controls->Add(this->tableLayoutPanel1);
      this->Name = L"Form1";
      this->Text = L"Form1";
      this->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &Form1::Form1_KeyDown);
      this->KeyUp += gcnew System::Windows::Forms::KeyEventHandler(this, &Form1::Form1_KeyUp);
      this->tableLayoutPanel1->ResumeLayout(false);
      this->tableLayoutPanel1->PerformLayout();
      this->ResumeLayout(false);

    }
#pragma endregion
	private: System::Void tableLayoutPanel1_Paint(System::Object^ sender, System::Windows::Forms::PaintEventArgs^ e) {
	}
private: System::Void Form1_KeyDown(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e) {
  if (e->KeyData == Keys::A)
  {
    this->oHWControl.sendHWControl("PitMenuDecrementValue", 1.0f);
  }
  else if (e->KeyData == Keys::D)
  {
    this->oHWControl.sendHWControl("PitMenuIncrementValue", 1.0f);
  }
  else if (e->KeyData == Keys::W)
  {
    this->oHWControl.sendHWControl("PitMenuUp", 1.0f);
  }
  else if (e->KeyData == Keys::S)
  {
    this->oHWControl.sendHWControl("PitMenuDown", 1.0f);
  }
  else
  {
    this->oHWControl.sendHWControl("ToggleMFDB", 1.0f);
  }
}

  private: System::Void Form1_KeyUp(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e) {
    if (e->KeyData == Keys::A)
    {
      this->oHWControl.sendHWControl("PitMenuDecrementValue", 0.0f);
    }
    else if (e->KeyData == Keys::D)
    {
      this->oHWControl.sendHWControl("PitMenuIncrementValue", 0.0f);
    }
    else if (e->KeyData == Keys::W)
    {
      this->oHWControl.sendHWControl("PitMenuUp", 0.0f);
    }
    else if (e->KeyData == Keys::S)
    {
      this->oHWControl.sendHWControl("PitMenuDown", 0.0f);
    }
    else
    {
      this->oHWControl.sendHWControl("ToggleMFDB", 0.0f);
    }
  }
};
}

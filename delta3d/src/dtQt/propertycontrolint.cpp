/* -*-c++-*-
 * testAPP - Using 'The MIT License'
 * Copyright (C) 2014, Caper Holdings LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

////////////////////////////////////////////////////////////////////////////////
// INCLUDE DIRECTIVES
////////////////////////////////////////////////////////////////////////////////
#include "ui_propertycontrolint.h"
#include <dtQt/propertycontrolint.h>



namespace dtQt
{
   /////////////////////////////////////////////////////////////////////////////
   // CLASS CODE
   /////////////////////////////////////////////////////////////////////////////
   PropertyControlInt::PropertyControlInt(QWidget* parent)
      : BaseClass(parent, dtCore::DataType::INT)
      , mUI(new Ui::PropertyControlInt)
   {
      mUI->setupUi(this);
   }

   PropertyControlInt::~PropertyControlInt()
   {
      delete mUI;
      mUI = NULL;
   }

   QObject* PropertyControlInt::GetControlUI()
   {
      return mUI->mLayout;
   }

   QLabel* PropertyControlInt::GetLabel() const
   {
      return mUI->mLabel;
   }

   void PropertyControlInt::CreateConnections()
   {
      BaseClass::CreateConnections();

      connect(mUI->mValue, SIGNAL(valueChanged(int)),
         this, SLOT(OnUIChanged()));
   }

   void PropertyControlInt::UpdateUI(const PropertyType& prop)
   {
      QString qstr(prop.ToString().c_str());
      int value = qstr.toInt();

      mUI->mValue->setValue(value);
   }

   void PropertyControlInt::UpdateData(PropertyType& propToUpdate)
   {
      propToUpdate.FromString(mUI->mValue->text().toStdString());
   }

}

#include <QDebug>
#include <QInputDialog>
#include <QListWidgetItem>
#include <QFileSystemModel>
#include <QJsonDocument>
#include <QGraphicsView>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileDialog>
#include <QList>
#include <QtGui>
#include <QDir>
#include <QSettings>
#include <QShortcut>

#include "MainWindow.h"
#include "ui_MainWindow.h"

MainWindow::~MainWindow() { delete ui; }

//--------------------------------------------------------------------------------------
MainWindow::MainWindow( QWidget *parent ) : QMainWindow( parent ), ui( new Ui::MainWindow )
{
    ui->setupUi( this );

    // Get children we'll need
    ui_imagePreview         = findChild<QLabel*>( "pixmapLabel" );
    ui_sizeLabel            = findChild<QLabel*>( "sizeSelectedLabel" );
    ui_nameLineEdit         = findChild<QLineEdit*>( "nameLineEdit" );
    ui_mainCatListWidget    = findChild<QListWidget*>( "mainCatListWidget" );
    ui_subCatListWidget     = findChild<QListWidget*>( "subCatListWidget" );
    ui_subCatButtonLayout   = findChild<QFrame*>( "subCatButtonLayout_2" );
    ui_fileListWidget       = findChild<QListWidget*>( "fileListWidget" );
    ui_sizeSlider           = findChild<QSlider*>( "sizeSlider" );
    ui_colorComboBox        = findChild<QComboBox*>( "colorComboBox" );
    ui_yearComboBox         = findChild<QComboBox*>( "yearComboBox" );
    ui_brokenCheckBox       = findChild<QCheckBox*>( "brokenCheckBox" );
    ui_batteriesCheckBox    = findChild<QCheckBox*>( "batteriesCheckBox" );
    ui_missingCheckBox      = findChild<QCheckBox*>( "missingCheckBox" );
    ui_saveButton           = findChild<QPushButton*>( "saveButton" );
    ui_saveAsButton         = findChild<QPushButton*>( "saveAsButton" );

    ui_saveButton->setEnabled( false );

    // Shortcut - Save
    QShortcut *saveShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this);
    QObject::connect( saveShortcut, SIGNAL( activated() ), this, SLOT(on_saveButton_clicked()) );

    // Shortcut - Save As
    QShortcut *saveAsShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_S), this);
    QObject::connect( saveAsShortcut, SIGNAL( activated() ), this, SLOT(on_saveAsButton_clicked()) );

    // Shortcut - Load
    QShortcut *openShortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_O), this);
    QObject::connect( openShortcut, SIGNAL( activated() ), this, SLOT(on_loadButton_clicked()) );

    // Add years to combo box
    const int startingYear = 1960;
    for ( int i = startingYear; i <= QDate::currentDate().year(); i++ )
        ui_yearComboBox->addItem( QString::number( i ) );

    m_appStateData = new AppStateData();

    // Check for settings
    QSettings settings( "Jeremy Abel", "Spectra" );
    if ( settings.status() == QSettings::NoError )
    {
        m_lastOpenMetafile = settings.value( "lastMetaFile", "" ).toString();

        // Load from last metafile
        if ( m_lastOpenMetafile.length() > 0 )
            loadAndParseJsonFile( m_lastOpenMetafile );
    }
}


// MAIN CATEGORY LIST
//================================================================================================

//--------------------------------------------------------------------------------------
void MainWindow::on_mainAddButton_clicked()
{
    bool ok;
    QString text = QInputDialog::getText( this, tr("Add Category" ), tr( "Category Name:" ), QLineEdit::Normal, "", &ok );

    if ( ok && !text.isEmpty() )
    {
        // Make key
        int key = m_appStateData->listCategories->count() > 0 ? m_appStateData->listCategories->lastKey() + 1 : 0;

        // Add to category data list
        CategoryData *newCategory = new CategoryData();
        newCategory->name = text;
        m_appStateData->listCategories->insert( key, newCategory );

        // Add to UI list
        QListWidgetItem *newItem = new QListWidgetItem();
        newItem->setText( text );
        newItem->setData( Qt::UserRole, m_appStateData->listCategories->lastKey() );
        ui_mainCatListWidget->addItem( newItem );

        qDebug() << "Adding new category:" << text << "-" << m_appStateData->listCategories->lastKey();
    }
}


//--------------------------------------------------------------------------------------
void MainWindow::on_mainRemoveButton_clicked()
{
    if ( ui_mainCatListWidget->count() <= 0 )
        return;

    qDebug() << "on_mainRemoveButton_clicked():" << ui_mainCatListWidget->currentIndex().row();

    // Remove from app data
    QString removedCategory = ui_mainCatListWidget->currentItem()->text();
    QListWidgetItem* removedItem = ui_mainCatListWidget->currentItem();
    m_appStateData->listCategories->remove( removedItem->data( Qt::UserRole ).toInt() );

    // Find all images with that category and clear their category
    for ( int i = 0; i < m_appStateData->listImageData->count(); i++ )
    {
        ImageData* imgData = m_appStateData->listImageData->at( i );
        if ( imgData->category == removedCategory )
            imgData->category = "";
    }

    // Remove from UI list
    ui_mainCatListWidget->takeItem( ui_mainCatListWidget->currentRow() );

    // Disable subcategory buttons
    if ( ui_mainCatListWidget->count() <= 0 )
    {
        ui_subCatListWidget->clear();
        ui_subCatButtonLayout->setEnabled( false );
    }
}


//--------------------------------------------------------------------------------------
void MainWindow::on_mainCatListWidget_doubleClicked()
{
    // Set as editable
    QListWidgetItem *item = ui_mainCatListWidget->currentItem();
    item->setFlags( item->flags() | Qt::ItemIsEditable );
    ui_mainCatListWidget->editItem( ui_mainCatListWidget->currentItem() );
}


//--------------------------------------------------------------------------------------
void MainWindow::on_mainCatListWidget_itemChanged( QListWidgetItem *item )
{
    // Adjust name of corresponding entry in app_data
    CategoryData *catData = m_appStateData->listCategories->value( item->data( Qt::UserRole ).toInt() );

    // Check against old name
    QString oldName = catData->name;
    if ( oldName == item->text() )
        return;

    catData->name = item->text();

    // Find all images with this category and rename it accordingly
    for ( int i = 0; i < m_appStateData->listImageData->count(); i++ )
    {
        ImageData* imgData = m_appStateData->listImageData->at( i );
        if ( imgData->category == oldName )
            imgData->category = item->text();
    }

    qDebug() << "on_mainCatListWidget_itemChanged():" << oldName << "->" << catData->name;
}


//--------------------------------------------------------------------------------------
void MainWindow::on_mainCatListWidget_currentItemChanged( QListWidgetItem *current )
{
    if ( !current )
        return;

    // Update subcategory list
    ui_subCatListWidget->clear();
    CategoryData    *catData = m_appStateData->listCategories->value( current->data( Qt::UserRole ).toInt() );
    QList<QVariant> *subcats = m_appStateData->listCategories->value( current->data( Qt::UserRole ).toInt() )->subcategories;

    qDebug() << "on_mainCatListWidget_currentItemChanged:" << catData->name << "-" << current->data( Qt::UserRole ).toInt();

    // Populate subcategory ui list
    for ( int i = 0; i < subcats->length(); i++ )
    {
        QListWidgetItem *newItem = new QListWidgetItem();
        newItem->setText( subcats->at( i ).toString() );
        newItem->setData( Qt::UserRole, i );
        ui_subCatListWidget->addItem( newItem );
    }

    // Set current image's category
    if ( getCurrentImageData() )
        getCurrentImageData()->category = current->text();

    // Enable subcategory buttons
    ui_subCatButtonLayout->setEnabled( true );
}


// SUB-CATEGORY LIST
//================================================================================================

//--------------------------------------------------------------------------------------
void MainWindow::on_subAddButton_clicked()
{
    if ( ui_mainCatListWidget->currentRow() < 0 )
        return;

    bool ok;
    QString text = QInputDialog::getText( this, tr("Add Sub-Category" ), tr( "Sub-Category Name:" ), QLineEdit::Normal, "", &ok );

    if ( ok && !text.isEmpty() )
    {
        qDebug() << "Adding new sub-category:" << text;

        // Append to subcategory list
        QList<QVariant> *subcats = m_appStateData->listCategories->value( ui_mainCatListWidget->currentItem()->data( Qt::UserRole ).toInt() )->subcategories;
        subcats->append( QVariant( text ) );

        // Add new ui list item
        QListWidgetItem *newItem = new QListWidgetItem();
        newItem->setText( text );
        newItem->setData( Qt::UserRole, subcats->size() - 1 );
        ui_subCatListWidget->addItem( newItem );
    }
}


//--------------------------------------------------------------------------------------
void MainWindow::on_subRemoveButton_clicked()
{
    if ( ui_mainCatListWidget->currentRow() < 0 || ui_subCatListWidget->currentRow() < 0 )
        return;

    qDebug() << "on_subRemoveButton_clicked()";

    if ( ui_subCatListWidget->count() > 0 )
    {
        QString oldName = ui_subCatListWidget->currentItem()->text();

        // Remove from category data
        QList<QVariant> *subcats = m_appStateData->listCategories->value( ui_mainCatListWidget->currentItem()->data( Qt::UserRole ).toInt() )->subcategories;
        subcats->removeAt( subcats->indexOf( QVariant( ui_subCatListWidget->currentItem()->text() ) ) );

        // Find all images with that subcategory and blank it
        for ( int i = 0; i < m_appStateData->listImageData->count(); i++ )
        {
            ImageData* imgData = m_appStateData->listImageData->at( i );
            if ( imgData->subCategory == oldName )
                imgData->subCategory = "";
        }

        // Remove from subcategory ui list
        ui_subCatListWidget->takeItem( ui_subCatListWidget->currentIndex().row() );
    }
}


//--------------------------------------------------------------------------------------
void MainWindow::on_subCatListWidget_doubleClicked()
{
    // Set as editable
    QListWidgetItem *item = ui_subCatListWidget->currentItem();
    item->setFlags( item->flags() | Qt::ItemIsEditable );
    ui_subCatListWidget->editItem( ui_subCatListWidget->currentItem() );
}


//--------------------------------------------------------------------------------------
void MainWindow::on_subCatListWidget_itemChanged( QListWidgetItem *item )
{
    // Adjust name of corresponding entry in app_data
    CategoryData *catData       = m_appStateData->listCategories->value( ui_mainCatListWidget->currentItem()->data( Qt::UserRole ).toInt() );
    QList<QVariant> *subcatList = catData->subcategories;

    // Check against old name
    QString oldName = subcatList->at( item->data( Qt::UserRole ).toInt() ).toString();
    if ( oldName == item->text() )
        return;

    // Rename
    subcatList->replace( item->data( Qt::UserRole ).toInt(), QVariant( item->text() ) );

    // Find all images with this subcategory and rename it accordingly
    for ( int i = 0; i < m_appStateData->listImageData->count(); i++ )
    {
        ImageData* imgData = m_appStateData->listImageData->at( i );
        if ( imgData->subCategory == oldName )
            imgData->subCategory = item->text();
    }

    qDebug() << "on_subCatListWidget_itemChanged():" << oldName << "->" << subcatList->at( item->data( Qt::UserRole ).toInt() ).toString();
}


//--------------------------------------------------------------------------------------
void MainWindow::on_subCatListWidget_currentItemChanged( QListWidgetItem *current )
{
    if ( !current )
        return;

    qDebug() << "on_subCatListWidget_currentItemChanged(): " + current->text();

    if ( getCurrentImageData() )
        getCurrentImageData()->subCategory = current->text();
}



// MAIN FUNCTIONALITY
//================================================================================================

//--------------------------------------------------------------------------------------
void MainWindow::keyPressEvent( QKeyEvent *event )
{
    // Deal with delete keypress depending on what has focus
    if ( event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace )
    {
        QWidget* focusWidget = QApplication::focusWidget();

        // Main category list
        if ( focusWidget == ui_mainCatListWidget )
        {
            on_mainRemoveButton_clicked();
            return;
        }

        // Sub-category list
        if ( focusWidget == ui_subCatListWidget )
        {
            on_subRemoveButton_clicked();
            return;
        }

        // File list
        if ( focusWidget == ui_fileListWidget )
        {
            if ( ui_fileListWidget->count() <= 0 )
                return;

            // Remove from UI list
            ui_fileListWidget->takeItem( ui_fileListWidget->currentRow() );
        }
    }
}

//--------------------------------------------------------------------------------------
void MainWindow::on_addFilesButton_clicked()
{
    qDebug() << "on_addFilesButton_clicked()";

    QFileDialog dialog( this );
    dialog.setFileMode( QFileDialog::ExistingFiles );
    dialog.setNameFilter( tr( "Images (*.png *.jpg *.bmp)" ) );
    dialog.setViewMode( QFileDialog::List );

    // Call dialog and process selected files
    QStringList fileNames;
    if ( dialog.exec() )
    {
        fileNames = dialog.selectedFiles();

        for ( int i = 0; i < fileNames.length(); i++ )
        {
            // Get file name and path
            QFileInfo *fileInfo = new QFileInfo( fileNames.at( i ) );
            QListWidgetItem *newItem = new QListWidgetItem( fileInfo->fileName(), ui_fileListWidget );

            // Create stock image data
            ImageData *newImageData = new ImageData();
            newImageData->name         = fileInfo->fileName();
            newImageData->path         = fileNames.at(i);
            newImageData->category     = tr( "" );
            newImageData->subCategory  = tr( "" );
            newImageData->color        = tr( "Red" );
            newImageData->year         = tr( "1980" );
            newImageData->size         = 1;
            newImageData->broken       = false;
            newImageData->missingParts = false;
            newImageData->batteries    = false;

            m_appStateData->listImageData->append( newImageData );

            newItem->setData( Qt::UserRole, m_appStateData->listImageData->size() - 1 );
        }
    }
}


//--------------------------------------------------------------------------------------
void MainWindow::on_fileListWidget_itemSelectionChanged()
{
    ImageData *imgData = getCurrentImageData();

    // Reset if no image is found
    if ( !imgData )
    {
        ui_mainCatListWidget->setCurrentRow( -1 );
        ui_subCatListWidget->setCurrentRow( -1 );
        ui_yearComboBox->setCurrentIndex(   ui_yearComboBox->findText("1980") );
        ui_colorComboBox->setCurrentIndex(  ui_colorComboBox->findText("Red") );
        ui_nameLineEdit->setText(           tr("") );
        ui_sizeSlider->setValue(            1 );
        ui_brokenCheckBox->setChecked(      false );
        ui_missingCheckBox->setChecked(     false );
        ui_batteriesCheckBox->setChecked(   false );
        ui_imagePreview->clear();
        return;
    }

    // Find and set category
    QList<QListWidgetItem *> itemCategories = ui_mainCatListWidget->findItems( imgData->category, Qt::MatchExactly );
    if ( itemCategories.length() > 0 )
    {
        ui_mainCatListWidget->setCurrentItem( itemCategories.first() );

        // Find and set sub-category
        QList<QListWidgetItem *> itemSubCategories = ui_subCatListWidget->findItems( imgData->subCategory, Qt::MatchExactly );
        if ( itemSubCategories.length() > 0 )
        {
            ui_subCatListWidget->setCurrentItem( itemSubCategories.first() );
        }
        else
        {
            // Clear selection
            QItemSelectionModel *selectionModel = ui_subCatListWidget->selectionModel();
            selectionModel->clear();
        }
    }
    else
    {
        // Clear main selection
        QItemSelectionModel * selectionModel = ui_mainCatListWidget->selectionModel();
        selectionModel->clear();
        ui_subCatListWidget->clear();
    }

    // Parse into the rest of the ui
    ui_yearComboBox->setCurrentIndex( ui_yearComboBox->findText( imgData->year ) );
    ui_colorComboBox->setCurrentIndex( ui_colorComboBox->findText( imgData->color ) );
    ui_nameLineEdit->setText(           imgData->name );
    ui_sizeSlider->setValue(            imgData->size );
    ui_brokenCheckBox->setChecked(      imgData->broken );
    ui_missingCheckBox->setChecked(     imgData->missingParts );
    ui_batteriesCheckBox->setChecked(   imgData->batteries );
    ui_imagePreview->setPixmap(         QPixmap( imgData->path ) );

    qDebug() << "on_fileListWidget_itemSelectionChanged():" << imgData->path;
}


//--------------------------------------------------------------------------------------
void MainWindow::on_saveButton_clicked()
{
    qDebug() << "on_saveButton_clicked()";
    serializeAndSaveJsonFile( m_lastOpenMetafile );
}


//--------------------------------------------------------------------------------------
void MainWindow::on_saveAsButton_clicked()
{
    qDebug() << "on_saveAsButton_clicked()";

    // Prep file dialog
    QFileDialog dialog( this );
    dialog.setFileMode( QFileDialog::AnyFile );
    dialog.setNameFilter( tr( "Metadata File (*.meta)" ) );
    dialog.setViewMode( QFileDialog::List );
    dialog.setAcceptMode( QFileDialog::AcceptSave );

    if ( !dialog.exec() )
        return;

    serializeAndSaveJsonFile( dialog.selectedFiles().at( 0 ) );
}


//--------------------------------------------------------------------------------------
void MainWindow::on_loadButton_clicked()
{
    qDebug() << "on_loadButton_clicked()";

    // Prep file dialog
    QFileDialog dialog( this );
    dialog.setFileMode( QFileDialog::ExistingFile );
    dialog.setNameFilter( tr( "Metadata File (*.meta)" ) );
    dialog.setViewMode( QFileDialog::List );

    // Show dialog
    if ( !dialog.exec() )
        return;

    loadAndParseJsonFile( dialog.selectedFiles().at( 0 ) );
}


//--------------------------------------------------------------------------------------
void MainWindow::serializeAndSaveJsonFile( const QString path )
{
    // Serialize category data
    QJsonArray categoryJsonArray;
    QMap<int, CategoryData*>::const_iterator iter = m_appStateData->listCategories->constBegin();
    while ( iter != m_appStateData->listCategories->constEnd() )
    {
        categoryJsonArray.append( iter.value()->serializeToJson() );
        iter++;
    }

    // Serialize image data
    QJsonArray imageJsonArray;
    for ( int i = 0; i < m_appStateData->listImageData->count(); i++ )
        imageJsonArray.append( m_appStateData->listImageData->at( i )->serializeToJson() );

    // Put everything into a QJsonDocument
    QJsonObject metafileObj;
    metafileObj["categories"] = categoryJsonArray;
    metafileObj["images"] = imageJsonArray;
    QJsonDocument jsonDoc = QJsonDocument( metafileObj );

    // Write file
    QFile file;
    file.setFileName( path );
    file.open( QIODevice::WriteOnly | QIODevice::Text );
    file.write( jsonDoc.toJson() );

    qDebug() << "serializeAndSaveJsonFile() -" << path;

    // Save for later
    m_lastOpenMetafile = file.fileName();
    saveSettings();
}


//--------------------------------------------------------------------------------------
void MainWindow::loadAndParseJsonFile( const QString path )
{
    qDebug() << "loadAndParseJsonFile() -" << path;

    // Save for later
    m_loadedMetaFileName = path;

    // Load selected file
    QString jsonFromFile;
    QFile file;
    file.setFileName( path );
    file.open( QIODevice::ReadOnly | QIODevice::Text );
    jsonFromFile = file.readAll();
    file.close();

    // Get jsonObject out of jsonDoc
    QJsonDocument jsonDoc    = QJsonDocument::fromJson( jsonFromFile.toUtf8() );
    QJsonObject   jsonObject = jsonDoc.object();

    // Clear old app data
    ui_mainCatListWidget->clear();
    ui_subCatListWidget->clear();
    ui_fileListWidget->clear();
    m_appStateData->listCategories->clear();
    m_appStateData->listImageData->clear();

    // Parse categories
    QJsonArray categoriesArray = jsonObject.value( "categories" ).toArray();
    for ( int i = 0; i < categoriesArray.count(); i++ )
    {
        QJsonObject categoryObj = categoriesArray.at( i ).toObject();
        CategoryData *newCategoryData = new CategoryData();

        newCategoryData->name = categoryObj["name"].toString();

        // Add subcategories
        QList<QVariant> *subcats = new QList<QVariant>( categoryObj["subcategories"].toArray().toVariantList() );
        newCategoryData->subcategories = subcats;

        // Add to app data
        m_appStateData->listCategories->insert( i, newCategoryData ); //m_appStateData->listCategories->count(), newCategoryData );

        // Add categories to ui main category list
        QListWidgetItem *newItem = new QListWidgetItem();
        newItem->setText( categoryObj["name"].toString() );
        newItem->setData( Qt::UserRole, i );//m_appStateData->listCategories->count() - 1 );
        ui_mainCatListWidget->addItem( newItem );
    }

    // Parse images
    QJsonArray imagesArray = jsonObject.value( "images" ).toArray();
    for ( int i = 0; i < imagesArray.count(); i++ )
    {
        QJsonObject imageObj = imagesArray.at( i ).toObject();
        ImageData *newImageData = new ImageData();

        newImageData->name          = imageObj.value( "name" ).toString();
        newImageData->path          = imageObj.value( "path" ).toString();
        newImageData->category      = imageObj.value( "category" ).toString();
        newImageData->subCategory   = imageObj.value( "subcategory" ).toString();
        newImageData->color         = imageObj.value( "color" ).toString();
        newImageData->year          = imageObj.value( "year" ).toString();
        newImageData->size          = (int)imageObj.value( "size" ).toDouble();
        newImageData->broken        = imageObj.value( "broken" ).toBool();
        newImageData->missingParts  = imageObj.value( "missingParts" ).toBool();
        newImageData->batteries     = imageObj.value( "batteries" ).toBool();

        // Add to app data
        m_appStateData->listImageData->append( newImageData );

        // Add to file list ui
        QListWidgetItem *newItem = new QListWidgetItem();
        newItem->setText( newImageData->name );
        newItem->setData( Qt::UserRole, m_appStateData->listImageData->count() - 1);
        ui_fileListWidget->addItem( newItem );
    }

    // Enable save button
    ui_saveButton->setEnabled( true );

    // TODO: Set title bar to show meta file name

    saveSettings();
}


//--------------------------------------------------------------------------------------
void MainWindow::saveSettings()
{
    qDebug() << "saveSettings()";
    QSettings settings( "Jeremy Abel", "Spectra" );
    settings.setValue( "lastMetaFile", m_lastOpenMetafile );
}


//--------------------------------------------------------------------------------------
void MainWindow::on_nameLineEdit_textChanged()              { if ( getCurrentImageData() ) getCurrentImageData()->name           = ui_nameLineEdit->text(); }
void MainWindow::on_colorComboBox_currentIndexChanged()     { if ( getCurrentImageData() ) getCurrentImageData()->color          = ui_colorComboBox->currentText(); }
void MainWindow::on_yearComboBox_currentIndexChanged()      { if ( getCurrentImageData() ) getCurrentImageData()->year           = ui_yearComboBox->currentText(); }
void MainWindow::on_brokenCheckBox_toggled(bool checked)    { if ( getCurrentImageData() ) getCurrentImageData()->broken         = checked; }
void MainWindow::on_missingCheckBox_toggled(bool checked)   { if ( getCurrentImageData() ) getCurrentImageData()->missingParts   = checked; }
void MainWindow::on_batteriesCheckBox_toggled(bool checked) { if ( getCurrentImageData() ) getCurrentImageData()->batteries      = checked; }


//--------------------------------------------------------------------------------------
void MainWindow::on_sizeSlider_valueChanged( int value )
{
    switch ( value )
    {
        case 0: ui_sizeLabel->setText( tr( "Small" ) );     break;
        case 1: ui_sizeLabel->setText( tr( "Medium" ) );    break;
        case 2: ui_sizeLabel->setText( tr( "Large" ) );     break;
    }

    if ( getCurrentImageData() )
        getCurrentImageData()->size = value;
}


//--------------------------------------------------------------------------------------
ImageData* MainWindow::getCurrentImageData()
{
    if ( !ui_fileListWidget->currentItem() )
        return NULL;

    return m_appStateData->listImageData->at( ui_fileListWidget->currentItem()->data( Qt::UserRole ).toInt() );
}

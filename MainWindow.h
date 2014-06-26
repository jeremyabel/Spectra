#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryFile>

#include "AppStateData.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QSlider;
class QListWidget;
class QComboBox;
class QCheckBox;
class QLineEdit;
class QPushButton;
class QGraphicsView;
class QGraphicsScene;
class QListWidgetItem;
class QFrame;
QT_END_NAMESPACE

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow( QWidget *parent = 0 );
    ~MainWindow();

    void keyPressEvent( QKeyEvent *event );

private slots:

    void on_mainAddButton_clicked();
    void on_mainRemoveButton_clicked();
    void on_mainCatListWidget_doubleClicked();
    void on_mainCatListWidget_itemChanged(QListWidgetItem *item);
    void on_mainCatListWidget_currentItemChanged( QListWidgetItem *current );

    void on_subAddButton_clicked();
    void on_subRemoveButton_clicked();
    void on_subCatListWidget_doubleClicked();
    void on_subCatListWidget_itemChanged(QListWidgetItem *item);
    void on_subCatListWidget_currentItemChanged(QListWidgetItem *current);

    void on_saveButton_clicked();
    void on_saveAsButton_clicked();
    void on_loadButton_clicked();

    void on_addFilesButton_clicked();
    void on_fileListWidget_itemSelectionChanged();

    void on_colorComboBox_currentIndexChanged();
    void on_yearComboBox_currentIndexChanged();
    void on_sizeSlider_valueChanged( int value );
    void on_brokenCheckBox_toggled(bool checked);
    void on_missingCheckBox_toggled(bool checked);
    void on_batteriesCheckBox_toggled(bool checked);

private:

    void            loadAndParseJsonFile( const QString path );
    void            serializeAndSaveJsonFile( const QString path );
    void            saveSettings();
    ImageData*      getCurrentImageData();

    Ui::MainWindow  *ui;
    QLabel          *ui_imagePreview;
    QLabel          *ui_pathLabel;
    QGraphicsScene  *ui_graphicsScene;
    QLabel          *ui_sizeLabel;
    QListWidget     *ui_mainCatListWidget;
    QListWidget     *ui_subCatListWidget;
    QFrame          *ui_subCatButtonLayout;
    QListWidget     *ui_fileListWidget;
    QPushButton     *ui_saveAsButton;
    QPushButton     *ui_saveButton;
    QSlider         *ui_sizeSlider;
    QComboBox       *ui_colorComboBox;
    QComboBox       *ui_yearComboBox;
    QCheckBox       *ui_brokenCheckBox;
    QCheckBox       *ui_missingCheckBox;
    QCheckBox       *ui_batteriesCheckBox;

    // Temp file things
    QTemporaryFile  m_tempFile;
    QString         m_loadedMetaFileName;
    QJsonObject     m_tempJson;
    QJsonArray      m_fileListJsonArray;
    QString         m_lastOpenMetafile;

    AppStateData    *m_appStateData;
};

#endif // MAINWINDOW_H

#include "DataManagerWidget.h"
#include "ui_DataManagerWidget.h"
#include <QVBoxLayout>
#include <QSpacerItem>
#include "MeshDataWidget.h"
#include "FieldDataWidget.h"
#include "VolumeDataWidget.h"
#include "MainWindow.h"

DataManagerWidget::DataManagerWidget(QWidget *parent) :
    QDockWidget(parent),
    ui(new Ui::DataManagerWidget)
{
    ui->setupUi(this);

    updateGroupWidgets();


    QObject::connect(MainWindow::dataManager(), SIGNAL(dataChanged()), this, SLOT(updateList()));
    QObject::connect(MainWindow::dataManager(), SIGNAL(selectionChanged()), this, SLOT(selectionUpdate()));
}

DataManagerWidget::~DataManagerWidget()
{
    delete ui;
}

DataGroupWidget* DataManagerWidget::makeNewGroup(DataGroup *group)
{
    DataGroupWidget *new_group = new DataGroupWidget();  // (parent, group)

    return new_group;
}

void DataManagerWidget::updateGroupWidgets()
{
    // Get a list of all the groups
    std::vector< DataGroup* > groups;

    //DataManager::Instance()->getGroups( groups );

    // Make a copy of the old widgets map
    DataWidgetMap tmp_map = this->groupMap;

    // Clear the original map
    this->groupMap.clear();

    // Loop through current groups and put their widgets in order.
    // Create new widgets when necessary

    for( size_t i = 0; i < groups.size(); i++ )
    {
        // Look for an existing widget for the group.
        // If found, remove it from temp map.
        //Otherwise, create new widget.

        //DataGroupWidget* groupWidget;
        std::string groupID; //= groups[i]->getGroupID();
        DataWidgetMap::iterator it = tmp_map.find(groupID);

        if (it != tmp_map.end())
        {

        }
        else
        {

        }



    }

    //FieldDataWidget *entry1 = new FieldDataWidget(this);
    //FieldDataWidget *entry2 = new FieldDataWidget(this);
    //DataObjectFrame *entry1 = new DataObjectFrame(this);    
    //DataObjectFrame *entry2 = new DataObjectFrame(this);
    //DataObjectFrame *entry3 = new DataObjectFrame(this);
    //DataObjectFrame *entry4 = new DataObjectFrame(this);
    //DataObjectFrame *entry5 = new DataObjectFrame(this);
    //DataObjectFrame *entry6 = new DataObjectFrame(this);
    spacer = new QSpacerItem(0,0, QSizePolicy::Expanding, QSizePolicy::Expanding);

    //entry2->showInfoClicked(true);

    // Put the widget in the layout
    vbox = new QVBoxLayout;
    //vbox->addWidget(entry1);
    //vbox->addWidget(entry2);
    //vbox->addWidget(entry3);
    //vbox->addWidget(entry4);
    //vbox->addWidget(entry5);
    //vbox->addWidget(entry6);
    vbox->addSpacerItem(spacer);
    vbox->setContentsMargins(0,0,0,0);
    vbox->setMargin(0);
    vbox->setSpacing(0);
    vbox->setAlignment(Qt::AlignTop);
    //vbox->set



    ui->scrollAreaWidgetContents->setLayout(vbox);
}

void DataManagerWidget::updateList()
{
    std::cout << "Updating Data List" << std::endl;

    // make copies of the old maps
    VolumeMap tmp_volume_map = this->volumeMap;
    FieldMap  tmp_field_map = this->fieldMap;
    MeshMap   tmp_mesh_map = this->meshMap;


    // clear the original maps
    this->volumeMap.clear();
    this->fieldMap.clear();
    this->meshMap.clear();


    // remove ending spacer temporarily
    vbox->removeItem(spacer);

    //-----------------------------------------------------------------
    //  Update Fields
    //-----------------------------------------------------------------
    for(size_t i=0; i < MainWindow::dataManager()->fields().size(); i++)
    {
        cleaver::AbstractScalarField *field = MainWindow::dataManager()->fields()[i];
        FieldDataWidget *fieldWidget = NULL;

        FieldMap::iterator it = tmp_field_map.find(reinterpret_cast<ulong>(field));
        if (it != tmp_field_map.end())
        {
            //std::cout << "Found Existing" << std::endl;
            fieldWidget = it->second;
            tmp_field_map.erase(it);
        }
        else{
            //std::cout << "Created New" << std::endl;
            fieldWidget = new FieldDataWidget(field, this);
        }

        // Put the widget in the layout
        vbox->insertWidget(i, fieldWidget);
        // Add the widget to the map
        fieldMap[reinterpret_cast<ulong>(field)] = fieldWidget;
    }

    // For anything left in temporary map, they are no longer needed.
    // Remove them from the layout and delete them.
    for(FieldMap::iterator it = tmp_field_map.begin(); it != tmp_field_map.end(); ++it)
    {
        FieldDataWidget *fieldWidget = it->second;
        vbox->removeWidget(fieldWidget);
        delete fieldWidget;
    }

    //-----------------------------------------------------------------
    //  Update Volumes
    //-----------------------------------------------------------------
    for(size_t i=0; i < MainWindow::dataManager()->volumes().size(); i++)
    {
        cleaver::Volume  *volume = MainWindow::dataManager()->volumes()[i];
        VolumeDataWidget *volumeWidget =  NULL;

        VolumeMap::iterator it = tmp_volume_map.find(reinterpret_cast<ulong>(volume));
        if (it != tmp_volume_map.end())
        {
            // found existing
            volumeWidget = it->second;

            //-- update it if necessary
            volumeWidget->updateFields();


            tmp_volume_map.erase(it);
        }
        else{
            std::cout << "Created New" << std::endl;
            volumeWidget = new VolumeDataWidget(volume, this);
        }

        // Put the widget in the layout
        vbox->insertWidget(i, volumeWidget);

        // Add the widget to the map
        volumeMap[reinterpret_cast<ulong>(volume)] = volumeWidget;
    }

    // For anything left in temporary map, they are no longer needed.
    // Remove them from the layout and delete them.
    for(VolumeMap::iterator it = tmp_volume_map.begin(); it != tmp_volume_map.end(); ++it)
    {
        VolumeDataWidget *volumeWidget = it->second;
        vbox->removeWidget(volumeWidget);
        delete volumeWidget;
    }

    //-----------------------------------------------------------------
    //  Update Meshes
    //-----------------------------------------------------------------
    for(size_t i=0; i < MainWindow::dataManager()->meshes().size(); i++)
    {
        cleaver::TetMesh *mesh = MainWindow::dataManager()->meshes()[i];
        MeshDataWidget *meshWidget =  NULL;

        MeshMap::iterator it = tmp_mesh_map.find(reinterpret_cast<ulong>(mesh));
        if (it != tmp_mesh_map.end())
        {
            std::cout << "Found Existing" << std::endl;
            meshWidget = it->second;
            tmp_mesh_map.erase(it);
        }
        else{
            std::cout << "Created New" << std::endl;
            meshWidget = new MeshDataWidget(mesh, this);
        }

        // Put the widget in the layout
        vbox->insertWidget(i, meshWidget);
        // Add the widget to the map
        meshMap[reinterpret_cast<ulong>(mesh)] = meshWidget;
    }

    // For anything left in temporary map, they are no longer needed.
    // Remove them from the layout and delete them.
    for(MeshMap::iterator it = tmp_mesh_map.begin(); it != tmp_mesh_map.end(); ++it)
    {
        MeshDataWidget *meshWidget = it->second;
        vbox->removeWidget(meshWidget);
        delete meshWidget;
    }



    // add spacer back
    vbox->addSpacerItem(spacer);
}

void DataManagerWidget::selectionUpdate()
{
    std::vector<ulong> selection = MainWindow::dataManager()->getSelection();

    //----------------------------------------------------
    //           Update Mesh Selections
    //----------------------------------------------------
    MeshMap::iterator mesh_iter = meshMap.begin();
    while(mesh_iter != meshMap.end())
    {
        MeshDataWidget *meshWidget = mesh_iter->second;

        bool found = false;
        for(size_t i=0; i < selection.size(); i++)
        {
            ulong ptr = selection[i];

            if(mesh_iter->first == ptr)
            {
                meshWidget->setSelected(true);
                found = true;
                break;
            }
        }
        if(!found)
            meshWidget->setSelected(false);

        mesh_iter++;
    }

    //----------------------------------------------------
    //           Update Volume Selections
    //----------------------------------------------------

    VolumeMap::iterator volume_iter = volumeMap.begin();
    while(volume_iter != volumeMap.end())
    {
        VolumeDataWidget *volumeWidget = volume_iter->second;
        bool found = false;

        found = false;
        for(size_t i=0; i < selection.size(); i++)
        {
            ulong ptr = selection[i];

            if(volume_iter->first == ptr)
            {
                volumeWidget->setSelected(true);
                found = true;
                break;
            }
        }
        if(!found)
            volumeWidget->setSelected(false);

        volume_iter++;
    }

    //----------------------------------------------------
    //          Update Field Selections
    //----------------------------------------------------
    FieldMap::iterator field_iter = fieldMap.begin();
    while(field_iter != fieldMap.end())
    {
        FieldDataWidget *fieldWidget = field_iter->second;
        bool found = false;

        found = false;
        for(size_t i=0; i < selection.size(); i++)
        {
            ulong ptr = selection[i];

            if(field_iter->first == ptr)
            {
                fieldWidget->setSelected(true);
                found = true;
                break;
            }
        }
        if(!found)
            fieldWidget->setSelected(false);

        field_iter++;
    }
}




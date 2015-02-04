#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <sstream>

#include <QtSql>
#include <QStandardItemModel>

#include "pricedialog.h"

void MainWindow::makeProductConnections()
{
    QObject::connect(this->ui->prod_newprice,
                     SIGNAL(clicked()),
                     this,
                     SLOT(showNewPricePopup()));
    QObject::connect(this->ui->prod_curprod_buttons,
                     SIGNAL(clicked(QAbstractButton*)),
                     this,
                     SLOT(generalProdButtonClicked(QAbstractButton*)));
}

void MainWindow::buildProductTree()
{
    QStandardItemModel * rootModel = new QStandardItemModel(this);

    QSqlQuery queryCat;
    QSqlQuery querySubcat;
    QSqlQuery queryProduct;

    std::stringstream query_str;

    query_str << "SELECT id, name FROM prod_cat" << std::endl;
    queryCat.exec(query_str.str().c_str());

    while (queryCat.next())
    {
        QStandardItem * category = new QStandardItem(queryCat.value("name").toString());
        int catId = queryCat.value("id").toInt();
        category->setData(QVariant::fromValue((int) SK_Category), SK_TypeRole);
        category->setData(QVariant::fromValue(catId), SK_IdRole);
        rootModel->appendRow(category);

        query_str.str(std::string());
        query_str.clear();
        query_str << "SELECT id, name FROM prod_subcat" << std::endl;
        query_str << "WHERE cat = " << catId << std::endl;
        querySubcat.exec(query_str.str().c_str());

        while (querySubcat.next())
        {
            QStandardItem * subcategory = new QStandardItem(querySubcat.value("name").toString());
            int subCatId = querySubcat.value("id").toInt();
            subcategory->setData(QVariant::fromValue((int) SK_Subcategory), SK_TypeRole);
            subcategory->setData(QVariant::fromValue(subCatId), SK_IdRole);
            category->appendRow(subcategory);

            query_str.str(std::string());
            query_str.clear();
            query_str << "SELECT id, name FROM product" << std::endl;
            query_str << "WHERE cat = " << catId << std::endl;
            query_str << "AND subcat = " << subCatId << std::endl;
            queryProduct.exec(query_str.str().c_str());

            while (queryProduct.next())
            {
                QStandardItem * product = new QStandardItem(queryProduct.value("name").toString());
                product->setData(QVariant::fromValue((int) SK_Product), SK_TypeRole);
                product->setData(QVariant::fromValue(queryProduct.value("id").toInt()), SK_IdRole);
                subcategory->appendRow(product);
            }
        }

//        query_str.str(std::string());
//        query_str.clear();
//        query_str << "SELECT id, name FROM product" << std::endl;
//        query_str << "WHERE cat = " << catId << std::endl;
//        query_str << "AND subcat IS NULL" << std::endl;
//        queryProduct.exec(query_str.str().c_str());

//        while (queryProduct.next())
//        {
//            QStandardItem * product = new QStandardItem(queryProduct.value("name").toString());
//            product->setData(QVariant::fromValue((int) SK_Product), SK_TypeRole);
//            product->setData(QVariant::fromValue(queryProduct.value("id").toInt()), SK_IdRole);
//            category->appendRow(product);
//        }
    }

    this->ui->prod_tree->setModel(rootModel);

}


void MainWindow::prodTreeItemSelected(const QModelIndex &current, const QModelIndex &previous)
{
    int type = current.data(SK_TypeRole).toInt();
    int id = current.data(SK_IdRole).toInt();

    if (type == SK_Product)
    {
        productSelected(id);
    }
}

void MainWindow::productSelected(int id)
{
    this->currentProduct = id;
    this->ui->prod_scroll->setEnabled(true);

    QSqlQuery query;
    query.prepare("SELECT name, notes, cat, subcat, meas "
                  "FROM product "
                  "WHERE id = :id" );
    query.bindValue(":id", id);
    query.exec();
    if (query.next())
    {
        this->ui->prod_name->setText(query.value("name").toString());
        this->ui->prod_notes->setPlainText(query.value("notes").toString());
        int cat = -1;
        int subcat = -1;
        int meas = -1;
        cat = (query.value("cat").isNull()) ? -1 : query.value("cat").toInt();
        subcat = (query.value("subcat").isNull()) ? -1 : query.value("subcat").toInt();
        meas = (query.value("meas").isNull()) ? -1 : query.value("meas").toInt();
        fillProductCategoryLists(cat, subcat);
        fillProductMeasurementList(meas);
        updatePriceList();
    }
}

void MainWindow::updatePriceList()
{
    QSqlQueryModel *pricemodel = new QSqlQueryModel();
    QSqlQuery pricequery;
    std::stringstream query_str;
    query_str << "SELECT price, notes FROM prod_price" << std::endl;
    query_str << "WHERE product = " << this->currentProduct << std::endl;
    pricequery.exec(query_str.str().c_str());
    pricemodel->setQuery(pricequery);
    this->ui->prod_pricetable->setModel(pricemodel);
}

void MainWindow::fillProductCategoryLists(int catId, int subCatId)
{
    this->ui->prod_cat->clear();
    this->ui->prod_subcat->clear();

    QSqlQuery query;
    std::stringstream query_str;
    query_str << "SELECT id, name FROM prod_cat" << std::endl;
    query.exec(query_str.str().c_str());
    int selected = 0, index = 1;
    this->ui->prod_cat->addItem("None", QVariant::fromValue(-1));
    while (query.next())
    {
        this->ui->prod_cat->addItem(query.value("name").toString(), QVariant::fromValue(query.value("id").toInt()));
        if (query.value("id").toInt() == catId)
            selected = index;
        index++;
    }
    this->ui->prod_cat->setCurrentIndex(selected);

    if (catId != -1)
    {
        QSqlQuery query;
        std::stringstream query_str;
        query_str << "SELECT id, name FROM prod_subcat" << std::endl;
        query_str << "WHERE cat = " << catId << std::endl;
        query.exec(query_str.str().c_str());
        int selected = 0, index = 1;
        this->ui->prod_subcat->addItem("None", QVariant::fromValue(-1));
        while (query.next())
        {
            this->ui->prod_subcat->addItem(query.value("name").toString(), QVariant::fromValue(query.value("id").toInt()));
            if (query.value("id").toInt() == subCatId)
                selected = index;
            index++;
        }
        this->ui->prod_subcat->setCurrentIndex(selected);
    }
    else
    {
        this->ui->prod_subcat->addItem("None", QVariant::fromValue(-1));
    }
}

void MainWindow::fillProductMeasurementList(int measId)
{
    this->ui->prod_meas->clear();

    QSqlQuery query;
    query.prepare("SELECT id, name FROM prod_meas");
    query.exec();
    int selected = 0, index = 1;
    this->ui->prod_meas->addItem("None", QVariant::fromValue(-1));
    while (query.next())
    {
        this->ui->prod_meas->addItem(query.value("name").toString(),
                                     QVariant::fromValue(query.value("id").toInt()));
        if (query.value("id").toInt() == measId)
            selected = index;
        index++;
    }
    this->ui->prod_meas->setCurrentIndex(selected);
}

void MainWindow::showNewPricePopup()
{
    PriceDialog* popUp = new PriceDialog(this);
    popUp->product(currentProduct);
    popUp->setModal(true);
    popUp->show();
}

void MainWindow::generalProdButtonClicked(QAbstractButton *button)
{
    switch (this->ui->prod_curprod_buttons->standardButton(button))
    {
    case QDialogButtonBox::Reset:
        resetProductData();
        break;
    }
}

void MainWindow::resetProductData()
{
    productSelected(currentProduct);
}

